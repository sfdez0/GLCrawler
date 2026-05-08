#include <GPO_assimp_aux.h>

#include "key_module.h"
#include "lighting.h"

namespace key_module {
    namespace {
        GLuint prog_key = 0;

        // Texturas que componen el material de la llave.
        struct MatTextures {
            GLuint base, normal, metallic, ao;
        };
        MatTextures mat_key;

        // Estructura devuelta por cargar_modelo_assimp con la malla de la llave
        struct escena key_model;

        // Parámetros de la animación de la llave.
        constexpr float HOVER_HEIGHT = 2.0f; // Altura base sobre el suelo (unidades de mundo)
        constexpr float HOVER_AMPLITUDE = 0.25f; // Amplitud del flotado vertical
        constexpr float HOVER_SPEED = 2.0f; // Frecuencia angular del flotado 
        constexpr float ROTATION_SPEED = 1.5f; // Velocidad de giro alrededor del eje Y 

        // Vincula las 4 texturas del material a sus unidades de textura.
        void bind_material(const MatTextures& m) {
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m.base);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m.normal);
            glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m.metallic);
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m.ao);
        }

        #define GLSL(src) "#version 330 core\n" #src

        const char* vertex_key = GLSL(
            layout(location = 0) in vec3 pos; // Posición del vértice
            layout(location = 1) in vec3 normal; // Normal del vértice 
            layout(location = 2) in vec2 uv; // Coordenadas de textura

            // Variables de salida hacia el fragment shader 
            out vec2 UV; 
            out vec3 FragPos;
            out vec3 Normal;

            // Uniforms compartidos por todos los vértices
            uniform mat4 MVP; 
            uniform mat4 M;

            void main(){
                gl_Position = MVP * vec4(pos, 1.0);
                FragPos = vec3(M * vec4(pos, 1.0));
                Normal = normalize(mat3(transpose(inverse(M))) * normal);
                UV = uv;
            }
        );

        const char* fragment_key = GLSL(
            in vec2 UV;
            in vec3 FragPos;
            in vec3 Normal;

            // Mapas que definen el material de la llave
            uniform sampler2D baseColorMap; // Color difuso base (sRGB)
            uniform sampler2D normalMap; // Normales en espacio tangente
            uniform sampler2D metallicMap; // Metalicidad (0 = dieléctrico, 1 = metal)
            uniform sampler2D aoMap; // Oclusión ambiente
            uniform float normalStrength; // Multiplicador para acentuar el relieve

            // Información de las luces de la escena 
            uniform int numLights; // Nº real de luces activas (<= 16)
            uniform vec3 lightPositions[16]; // Posiciones en coords. de mundo
            uniform vec3 lightColors[16]; // Color/intensidad RGB de cada luz
            uniform vec3 camPos; // Posición de la cámara 

            out vec3 outputColor;

            const float light_range = 8.0f;
            const float light_soft = 2.5f;

            mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv){
                // Derivadas de la posición y de las UV respecto a las coords. de pantalla
                vec3 dp1 = dFdx(p);
                vec3 dp2 = dFdy(p);
                vec2 duv1 = dFdx(uv);
                vec2 duv2 = dFdy(uv);

                // Vectores perpendiculares para construir tangente y bitangente
                vec3 dp2perp = cross(dp2, N);
                vec3 dp1perp = cross(N, dp1);
                vec3 T = dp2perp * duv1.x + dp1perp * duv2.x; 
                vec3 B = dp2perp * duv1.y + dp1perp * duv2.y; 

                // Normalizamos T y B con el mismo factor para no deformar la base
                float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
                return mat3(T * invmax, B * invmax, N);
            }

            void main(){

                vec3 baseColor = pow(texture(baseColorMap, UV).rgb, vec3(2.2));
                float metallic = texture(metallicMap, UV).r;
                float ao = texture(aoMap, UV).r;
                float baseRough = mix(0.65, 0.18, metallic);
                float roughness = clamp(baseRough, 0.05, 0.95);

                vec3 F0 = mix(vec3(0.04), baseColor, metallic);

                vec3 diffuseColor = baseColor * (1.0 - metallic);


                vec3 V = normalize(camPos - FragPos);
                vec3 Ngeom = normalize(Normal);
                vec3 nTan = texture(normalMap, UV).rgb * 2.0 - 1.0; 
                nTan.xy *= normalStrength; 
                vec3 N = normalize(cotangent_frame(Ngeom, FragPos, UV) * nTan);

                float NdotV = max(dot(N, V), 0.001);

                float shininess = mix(8.0, 256.0, 1.0 - roughness);

                vec3 R = reflect(-V, N);
                vec3 skyColor = vec3(0.55, 0.40, 0.20);
                vec3 floorColor = vec3(0.08, 0.05, 0.03);
                vec3 envColor = mix(floorColor, skyColor, R.y * 0.5 + 0.5);

                vec3 F_amb = F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);

                vec3 ambient = diffuseColor * 0.06
                            + envColor * F_amb * (1.0 - roughness) * 0.5;
                ambient *= ao;

                // Inicializamos el color final con la contribución ambiente
                vec3 result = ambient;

                // Iteramos sobre todas las luces de la escena (antorchas + luz de la llave)
                for (int i = 0; i < numLights; i++){
                    // Vector luz-fragmento y su distancia
                    vec3 Lvec = lightPositions[i] - FragPos;
                    float light_dist = length(Lvec);
                    if (light_dist > light_range + light_soft) continue; // Si el fragmento está fuera del rango, saltamos esta luz

                    vec3 L = Lvec / max(light_dist, 0.0001); // Normalizado, evitando /0

                    // Ángulo luz-normal para la difusa
                    float NdotL = max(dot(N, L), 0.0);

                    vec3 H = normalize(L + V);
                    float NdotH = max(dot(N, H), 0.0);
                    float VdotH = max(dot(V, H), 0.0);

                    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

                    // Componente especular
                    float specular = pow(NdotH, shininess) * (shininess + 8.0) / 25.0;
                    vec3 specComp = F * specular;

                    // Atenuación con la distancia
                    float cutoff = 1.0 - smoothstep(light_range - light_soft, light_range, light_dist);
                    float att = cutoff / (1.0 + 0.15 * light_dist + 0.20 * light_dist * light_dist);

                    // Contribución total de esta luz al fragmento
                    vec3 contrib = (diffuseColor + specComp) * NdotL * lightColors[i] * att;
                    result += contrib;
                }

                // Corrección gamma final 
                result = pow(result, vec3(1.0 / 2.2));

                outputColor = result;
            }
        );
    } 

    void init() {
        // Compilación y enlazado del programa de shaders propio de la llave
        GLuint VertexKeyID = compilar_shader(vertex_key, GL_VERTEX_SHADER);
        GLuint FragmentKeyID = compilar_shader(fragment_key, GL_FRAGMENT_SHADER);

        prog_key = glCreateProgram();
        glAttachShader(prog_key, VertexKeyID);
        glAttachShader(prog_key, FragmentKeyID);
        glLinkProgram(prog_key);
        check_errores_programa(prog_key);

        glDetachShader(prog_key, VertexKeyID);
        glDeleteShader(VertexKeyID);
        glDetachShader(prog_key, FragmentKeyID);
        glDeleteShader(FragmentKeyID);

        glUseProgram(prog_key);

        // Carga de las 4 texturas del material en sus unidades correspondientes
        mat_key.base = cargar_textura_rgba("bin/data/key/key_color.png", GL_TEXTURE0);
        mat_key.normal = cargar_textura_rgba("bin/data/key/key_nmap.png", GL_TEXTURE1);
        mat_key.metallic = cargar_textura_rgba("bin/data/key/key_metalness.png", GL_TEXTURE2);
        mat_key.ao = cargar_textura_rgba("bin/data/key/key_ao.png", GL_TEXTURE3);

        // Indicamos al shader qué unidad de textura corresponde a cada sampler.
        transfer_int("baseColorMap", 0);
        transfer_int("normalMap", 1);
        transfer_int("metallicMap", 2);
        transfer_int("aoMap", 3);
        transfer_float("normalStrength", 1.5f);

        // Carga del modelo 3D. assimp parsea el .obj y devuelve los VAOs/VBOs
        key_model = cargar_modelo_assimp("bin/data/key/key.obj");
    }

    // draw(): dibuja una llave en la posición indicada, animada según el tiempo. 
    void draw(vec3 base_pos, float scale, float time, mat4 P, mat4 V, vec3 cam_pos) {
        // Oscilación senoidal en altura + giro continuo sobre eje Y
        float hover = HOVER_AMPLITUDE * sin(time * HOVER_SPEED);
        float angle = time * ROTATION_SPEED;
        vec3 anim_pos = base_pos + vec3(0.0f, HOVER_HEIGHT + hover, 0.0f);

        mat4 M = translate(mat4(1.0f), anim_pos)
               * rotate(mat4(1.0f), angle, vec3(0,1,0))
               * rotate(mat4(1.0f), glm::radians(45.0f), vec3(0,0,1))
               * glm::scale(mat4(1.0f), vec3(scale, scale, scale));

        lighting::upload_to_shader(prog_key); // Incluye "glUseProgram(prog_key);"

        // Subida de uniforms al shader
        transfer_mat4("MVP", P * V * M); 
        transfer_mat4("M", M);
        transfer_vec3("camPos", cam_pos);

        // Vinculamos las texturas del material a sus unidades antes de dibujar
        bind_material(mat_key);

        for (unsigned int i = 0; i < key_model.nInstancias; i++){
            unsigned int j = key_model.instIdx[i];
            objeto& obj = key_model.objs[j];
            glBindVertexArray(obj.VAO);
            glDrawElements(GL_TRIANGLES, obj.Ni, obj.tipo_indice, (void*)0);
        }
        
        glBindVertexArray(0);
    }

    // Convierte coordenadas (x,y) del grid del laberinto acoordenadas de mundo XZ centradas en el origen
    vec3 compute_world_pos(int x, int y, float tile_size, float maze_center_xz) {
        float cx = x * tile_size - maze_center_xz;
        float cz = y * tile_size - maze_center_xz;
        return vec3(cx, 0.0f, cz);
    }

    // Posición donde colocar la luz puntual que emite la llave
    vec3 compute_light_pos(vec3 base_pos) {
        return base_pos + vec3(0.0f, HOVER_HEIGHT + 0.3f, 0.0f);
    }
} 
