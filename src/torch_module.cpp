#include "torch_module.h"
#include <GPO_assimp_aux.h>
#include <lighting.h>

namespace torch_module {

namespace {
    GLuint prog_torch = 0;

    // Texturas de antorcha
    struct MatTextures {
        GLuint base, normal, metallic, roughness;
    };
    MatTextures mat_torch; // mango central
    MatTextures mat_hanger; // placa de pared

    struct escena torch_model;

    // Configuración del posicionamiento 
    constexpr float WALL_OFFSET = -0.4f; // desplazamiento desde el centro de la celda hacia la pared
    constexpr float HEIGHT_FACTOR = 1.0f; // altura sobre el suelo

    // Enlazar 4 texturas a sus unidades GLSL
    void bind_material(const MatTextures& m) {
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m.base);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m.normal);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m.metallic);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m.roughness);
    }

    #define GLSL(src) "#version 330 core\n" #src

    // layout sigue el orden del cargador Assimp: pos = 0, normal = 1, uv = 2
    const char* vertex_torch = GLSL(
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

        const char* fragment_torch = GLSL(
        in vec2 UV;
        in vec3 FragPos;
        in vec3 Normal;

        // Mapas de material
        uniform sampler2D baseColorMap;
        uniform sampler2D normalMap;
        uniform sampler2D metallicMap;
        uniform sampler2D roughnessMap;
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

            // Parámetros derivados de Metallic/Roughness
            float shininess = mix(4.0, 128.0, 1.0 - roughness);
            float Ks = mix(0.05, 0.5, metallic);
            vec3 specTint = mix(vec3(1.0), baseColor, metallic);
            vec3 diffuseColor = baseColor * mix(1.0, 0.15, metallic);

            // Ambiente
            vec3 ambient = baseColor * mix(0.20, 0.12, metallic);
            vec3 result = ambient;

            float light_range = 6.0;
            float light_soft = 2.0;

            for (int i = 0; i < numLights; i++){
                vec3 L = lightPositions[i] - FragPos;
                float dist = length(L);
                L = normalize(L);

                // Difusa
                float diffuse = max(dot(N, L), 0.0);

                // Especular Phong + Fresnel boost
                vec3 R = reflect(-L, N);
                float specular = pow(max(dot(V, R), 0.0), shininess);

                // Atenuación de la escena
                float cutoff = 1.0 - smoothstep(light_range - light_soft, light_range, dist);
                float att = cutoff / (1.0 + 0.2 * dist + 0.5 * dist * dist);

                // Parpadeo 
                float intensity = 1.0;
                if (i > 0){
                    float seed = fract(i * 12.9898 + i * 78.233 - i * 45.164);
                    float phase = seed * 6.2831853;
                    intensity = 0.8
                              + 0.25 * sin(time * (1.1 + seed*0.9) + phase)
                              + 0.10 * sin(time * (2.7 + seed*1.3) + phase * 1.7)
                              + 0.05 * sin(time * (4.5 + seed*2.1) + phase * 2.3);
                    intensity = clamp(intensity, 0.5, 1.2);
                }

                // Especular
                vec3 specComponent = specTint * (Ks * specular * 0.4);

                vec3 contrib = ( diffuseColor * diffuse + specComponent )
                             * lightColors[i] * att * intensity;

                result += contrib;
            }

            result = pow(result, vec3(1.0 / 2.2));

            outputColor = result;
        }
    );
}

void init() {
    // Compilar shaders propios para la antorcha
	GLuint VertexTorchID = compilar_shader(vertex_torch, GL_VERTEX_SHADER);
    GLuint FragmentTorchID = compilar_shader(fragment_torch, GL_FRAGMENT_SHADER);

    prog_torch = glCreateProgram();
    glAttachShader(prog_torch, VertexTorchID);
    glAttachShader(prog_torch, FragmentTorchID);
    glLinkProgram(prog_torch);
    check_errores_programa(prog_torch);

    glDetachShader(prog_torch, VertexTorchID);
	glDeleteShader(VertexTorchID);
    glDetachShader(prog_torch, FragmentTorchID);
	glDeleteShader(FragmentTorchID);

    // Cambiar al programa de antorcha
    glUseProgram(prog_torch);

    // Carga de los 4 mapas por material
    mat_torch.base = cargar_textura_rgba("bin/data/torch/torch_lp_Torch_BaseColor.png", GL_TEXTURE0);
    mat_torch.normal = cargar_textura_rgba("bin/data/torch/torch_lp_Torch_Normal.png",    GL_TEXTURE1);
    mat_torch.metallic = cargar_textura_rgba("bin/data/torch/torch_lp_Torch_Metallic.png",  GL_TEXTURE2);
    mat_torch.roughness = cargar_textura_rgba("bin/data/torch/torch_lp_Torch_Roughness.png", GL_TEXTURE3);

    mat_hanger.base = cargar_textura_rgba("bin/data/torch/torch_hanger_lp_torch_hanger_BaseColor.png", GL_TEXTURE0);
    mat_hanger.normal = cargar_textura_rgba("bin/data/torch/torch_hanger_lp_torch_hanger_Normal.png",    GL_TEXTURE1);
    mat_hanger.metallic = cargar_textura_rgba("bin/data/torch/torch_hanger_lp_torch_hanger_Metallic.png",  GL_TEXTURE2);
    mat_hanger.roughness = cargar_textura_rgba("bin/data/torch/torch_hanger_lp_torch_hanger_Roughness.png", GL_TEXTURE3);

    // Mapeo samplers
    transfer_int("baseColorMap", 0);
    transfer_int("normalMap", 1);
    transfer_int("metallicMap", 2);
    transfer_int("roughnessMap", 3);
    transfer_float("normalStrength", 1.5f);

    torch_model = cargar_modelo_assimp("bin/data/torch/torch.obj");
}

void shutdown() {
    // Borrar programa
    if (prog_torch != 0) {
        glDeleteProgram(prog_torch);
        prog_torch = 0;
    }

    // Borrar texturas de antorcha
    if (mat_torch.base != 0) {
        glDeleteTextures(1, &mat_torch.base); 
        mat_torch.base = 0; 
    }
    if (mat_torch.normal != 0) {
        glDeleteTextures(1, &mat_torch.normal);
        mat_torch.normal = 0;
    }
    if (mat_torch.metallic != 0) {
        glDeleteTextures(1, &mat_torch.metallic);
        mat_torch.metallic = 0;
    }
    if (mat_torch.roughness != 0) {
        glDeleteTextures(1, &mat_torch.roughness);
        mat_torch.roughness = 0;
    }

    // Borrar texturas de hanger
    if (mat_hanger.base != 0) {
        glDeleteTextures(1, &mat_hanger.base);
        mat_hanger.base = 0;
    }
    if (mat_hanger.normal != 0) {
        glDeleteTextures(1, &mat_hanger.normal);
        mat_hanger.normal = 0;
    }
    if (mat_hanger.metallic != 0) {
        glDeleteTextures(1, &mat_hanger.metallic);
        mat_hanger.metallic = 0;
    }
    if (mat_hanger.roughness != 0) {
        glDeleteTextures(1, &mat_hanger.roughness);
        mat_hanger.roughness = 0;
    }

    // Borrar modelo
    if (torch_model.objs || torch_model.mats || torch_model.instIdx || torch_model.buffers) {
        limpiar_escena(&torch_model);
        torch_model = {};
    }
}

void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V, vec3 cam_pos) {
    mat4 M = translate(mat4(1.0f), pos)
           * rotate(mat4(1.0f), rot_y, vec3(0, 1, 0))
           * glm::scale(mat4(1.0f), vec3(scale, scale, scale));

    lighting::upload_to_shader(prog_torch); // Incluye "glUseProgram(prog_torch);"

    transfer_mat4 ("MVP", P * V * M);
    transfer_mat4 ("M", M);
    transfer_vec3 ("camPos", cam_pos);
    transfer_float("time", (float)glfwGetTime());

    for (unsigned int i = 0; i < torch_model.nInstancias; i++){
        unsigned int j = torch_model.instIdx[i];
        objeto& obj = torch_model.objs[j];

        // j == 0 mango ; j != 0 placa con grabado
        bind_material(j == 0 ? mat_torch : mat_hanger);
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
        case 'l': return glm::pi<float>(); // 180º
        case 'r': return 0.0f; // 0º
    }
    return 0.0f;
}

}