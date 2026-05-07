#include "door.h"
#include <GPO_assimp_aux.h>
#include <lighting.h>

namespace door {

    namespace {
        GLuint prog_door = 0;

        // Texturas de puerta
        struct MatTextures {
            GLuint base, normal, metallic, roughness, ao;
        };
        MatTextures mat_door; // mango central

        struct escena door_model;

        // Configuración del posicionamiento 
        constexpr float WALL_OFFSET = -0.4f; // desplazamiento desde el centro de la celda hacia la pared
        constexpr float HEIGHT_FACTOR = 0.6f; // altura sobre el suelo

        // Enlazar 4 texturas a sus unidades GLSL
        void bind_material(const MatTextures& m) {
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m.base);
            glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m.normal);
            glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m.metallic);
            glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m.roughness);
            glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, m.ao);
        }

        #define GLSL(src) "#version 330 core\n" #src

        // layout sigue el orden del cargador Assimp: pos = 0, normal = 1, uv = 2
        const char* vertex_door = GLSL(
            layout(location = 0) in vec3 pos;
            layout(location = 1) in vec3 normal;
            layout(location = 2) in vec2 uv;

            out vec2 UV;
            out vec3 FragPos;
            out vec3 Normal;

            uniform mat4 MVP;
            uniform mat4 M;

            void main(){
                gl_Position = MVP * vec4(pos, 1.0);
                FragPos = vec3(M * vec4(pos, 1.0));
                Normal = normalize(mat3(transpose(inverse(M))) * normal);
                UV = uv;
            }
        );

        const char* fragment_door = GLSL(
            in vec2 UV;
            in vec3 FragPos;
            in vec3 Normal;

            // Mapas de material
            uniform sampler2D baseColorMap;
            uniform sampler2D normalMap;
            uniform sampler2D metallicMap;
            uniform sampler2D roughnessMap;
            uniform sampler2D aoMap;
            uniform float normalStrength;

            // Luces
            uniform int numLights;
            uniform vec3 lightPositions[16];
            uniform vec3 lightColors[16];
            uniform vec3 camPos;
            uniform float time;

            out vec3 outputColor;

            mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv){
                vec3 dp1 = dFdx(p);
                vec3 dp2 = dFdy(p);
                vec2 duv1 = dFdx(uv);
                vec2 duv2 = dFdy(uv);

                vec3 dp2perp = cross(dp2, N);
                vec3 dp1perp = cross(N, dp1);
                vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
                vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

                float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
                return mat3(T * invmax, B * invmax, N);
            }

            void main(){
                vec3 baseColor = pow(texture(baseColorMap, UV).rgb, vec3(2.2));
                float metallic = texture(metallicMap, UV).r;
                float roughness = texture(roughnessMap, UV).r;

                vec3 V = normalize(camPos - FragPos);

                vec3 Ngeom = normalize(Normal);
                vec3 nTan = texture(normalMap, UV).rgb * 2.0 - 1.0;
                nTan.xy *= normalStrength;
                vec3 N = normalize(cotangent_frame(Ngeom, FragPos, UV) * nTan);

                // Cargamos ambient occlusion y aplicamos intensidad
                float ao = texture(aoMap, UV).r;
                ao = mix(1.0, ao, 1.0f * 0.5);

                // Parámetros derivados de Metallic/Roughness
                float shininess = mix(4.0, 128.0, 1.0 - roughness);
                float Ks = mix(0.05, 0.5, metallic);
                vec3 specTint = mix(vec3(1.0), baseColor, metallic);
                vec3 diffuseColor = baseColor * mix(1.0, 0.15, metallic);

                // Ambiente
                vec3 ambient = baseColor * mix(0.05, 0.02, metallic) * ao;
                vec3 result = ambient;

                float specularPower = mix(32.0, 2.0, 1.0f);

                float light_range = 6.0; // Rango máximo de la luz
                float light_soft = 2.0; // Rango de suavizado al final del rango máximo

                // Acumular contribución de cada antorcha
                for(int i = 0; i < numLights; i++){
                    vec3 L = lightPositions[i] - FragPos;
                    float light_dist = length(L);  // Distancia entre la luz y el fragmento
                    if (light_dist > light_range + light_soft) continue; // Si el fragmento está fuera del rango, saltamos esta luz

                    L = normalize(L);

                    //Difusa
                    float diffuse = max(dot(N,L), 0.0);

                    // Especular 
                    vec3 R = reflect(-L, N);
                    float specular = pow(max(dot(V, R), 0.0), specularPower) * 0.045;

                    // Calculos relativos a la luz del personaje
                    float light_cutoff = 1.0 - smoothstep(light_range - light_soft, light_range, light_dist); // Factor de atenuación basado en la distancia
                    float light_attenuation = light_cutoff / (1.0 + 0.2 * light_dist + 0.5 * light_dist * light_dist); // Atenuación

                    // Cambio pseudoaleatorio en la intensidad de cada luz para simular el parpadeo de las llamas
                    float intensity = 1.0;
                    if (i > 0) { // Excepto luz del personaje
                        float seed = fract(i * 12.9898 + i * 78.233 - i * 45.164); // Semilla basada en índice
                        float phase = seed * 6.2831853; // Fase "base"
                        float f1 = 1.1 + seed * 0.9; // Frecuencia 1
                        float f2 = 2.7 + seed * 1.3; // Frecuencia 2
                        float f3 = 4.5 + seed * 2.1; // Frecuencia 3

                        // Aplicamos a la intensidad la suma de 3 ondas sinusoidales pseudoaleatorias
                        intensity = 0.8
                            + 0.25 * sin(time * f1 + phase)
                            + 0.10 * sin(time * f2 + phase * 1.7)
                            + 0.05 * sin(time * f3 + phase * 2.3);

                        // Limitamos intensidad entre 0.5 y 1.2
                        intensity = clamp(intensity, 0.5, 1.2); 
                    }

                    vec3 contribution = (baseColor * ((ambient * light_cutoff) + 1.5 * diffuse * light_attenuation * ao) + vec3(specular * light_attenuation)) * lightColors[i];

                    result += contribution * intensity;
                }

                outputColor = result;
            }
        );
    }

    void init() {
        // Compilar shaders propios para la puerta
        GLuint VertexDoorID = compilar_shader(vertex_door, GL_VERTEX_SHADER);
        GLuint FragmentDoorID = compilar_shader(fragment_door, GL_FRAGMENT_SHADER);

        prog_door = glCreateProgram();
        glAttachShader(prog_door, VertexDoorID);
        glAttachShader(prog_door, FragmentDoorID);
        glLinkProgram(prog_door);
        check_errores_programa(prog_door);

        glDetachShader(prog_door, VertexDoorID);
        glDeleteShader(VertexDoorID);
        glDetachShader(prog_door, FragmentDoorID);
        glDeleteShader(FragmentDoorID);

        // Cambiar al programa de puerta
        glUseProgram(prog_door);

        // Carga de los 5 mapas por material
        mat_door.base = cargar_textura_rgba("bin/data/door/Door_noas_L_Door_BaseColor.tga.png", GL_TEXTURE0);
        mat_door.normal = cargar_textura_rgba("bin/data/door/Door_noas_L_Door_Normal.tga.png",    GL_TEXTURE1);
        mat_door.metallic = cargar_textura_rgba("bin/data/door/Door_noas_L_Door_Metallic.tga.png",  GL_TEXTURE2);
        mat_door.roughness = cargar_textura_rgba("bin/data/door/Door_noas_L_Door_Roughness.tga.png", GL_TEXTURE3);
        mat_door.ao = cargar_textura_rgba("bin/data/door/Door_noas_L_Door_AO.tga.png", GL_TEXTURE4);

        // Mapeo samplers
        transfer_int("baseColorMap", 0);
        transfer_int("normalMap", 1);
        transfer_int("metallicMap", 2);
        transfer_int("roughnessMap", 3);
        transfer_int("aoMap", 4);
        transfer_float("normalStrength", 1.5f);

        door_model = cargar_modelo_assimp("bin/data/door/door.obj");
    }

    void shutdown() {
        // Borrar programa
        if (prog_door != 0) {
            glDeleteProgram(prog_door);
            prog_door = 0;
        }

        // Borrar texturas de puerta
        if (mat_door.base != 0) {
            glDeleteTextures(1, &mat_door.base); 
            mat_door.base = 0; 
        }
        if (mat_door.normal != 0) {
            glDeleteTextures(1, &mat_door.normal);
            mat_door.normal = 0;
        }
        if (mat_door.metallic != 0) {
            glDeleteTextures(1, &mat_door.metallic);
            mat_door.metallic = 0;
        }
        if (mat_door.roughness != 0) {
            glDeleteTextures(1, &mat_door.roughness);
            mat_door.roughness = 0;
        }

        // Borrar modelo
        if (door_model.objs || door_model.mats || door_model.instIdx || door_model.buffers) {
            limpiar_escena(&door_model);
            door_model = {};
        }
    }

    void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V, vec3 cam_pos) {
        mat4 M = translate(mat4(1.0f), pos)
            * rotate(mat4(1.0f), rot_y, vec3(0, 1, 0))
            * glm::scale(mat4(1.0f), vec3(scale, scale, scale));

        lighting::upload_to_shader(prog_door); // Incluye "glUseProgram(prog_door);"

        transfer_mat4 ("MVP", P * V * M);
        transfer_mat4 ("M", M);
        transfer_vec3 ("camPos", cam_pos);
        transfer_float("time", (float)glfwGetTime());

        for (unsigned int i = 0; i < door_model.nInstancias; i++){
            unsigned int j = door_model.instIdx[i];
            objeto& obj = door_model.objs[j];

            bind_material(mat_door);
            glBindVertexArray(obj.VAO);
            glDrawElements(GL_TRIANGLES, obj.Ni, obj.tipo_indice, (void*)0);
        }
        glBindVertexArray(0);
    }

    vec3 compute_world_pos(int x, int y, char direction,
                        float tile_size, float maze_center_xz) {
        float wall_offset = tile_size * 0.5f + WALL_OFFSET; // Centro de celda
        float height = tile_size * HEIGHT_FACTOR; // Altura sobre el suelo

        float cx = x * tile_size - maze_center_xz;
        float cz = y * tile_size - maze_center_xz;
        float dx = 0.0f, dz = 0.0f;

        switch (direction) {
            case 'u': dz = -wall_offset; break;
            case 'd': dz = +wall_offset; break;
            case 'l': dx = -wall_offset; break;
            case 'r': dx = +wall_offset; break;
        }

        return vec3(cx + dx, height, cz + dz);
    }

    float compute_rotation(char direction) {
        switch (direction) {
            case 'u': return -glm::pi<float>() * 0.5f; // -90º
            case 'd': return glm::pi<float>() * 0.5f; // +90º
            case 'l': return 0.0f; // 0º
            case 'r': return glm::pi<float>(); // 180º
        }
        return 0.0f;
    }
}
