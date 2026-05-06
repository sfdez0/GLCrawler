#include "enemy.h"
#include <GPO_assimp_aux.h>
#include <lighting.h>

namespace enemy {

    namespace {
        GLuint prog_enemy = 0;

        struct escena enemy_model;

        // Configuración del posicionamiento 
        constexpr float HEIGHT_FACTOR = 0.6f; // altura sobre el suelo

        #define GLSL(src) "#version 330 core\n" #src

        // layout sigue el orden del cargador Assimp: pos = 0, normal = 1, uv = 2
        const char* vertex_enemy = GLSL(
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

        const char* fragment_enemy = GLSL(
            in vec3 FragPos;
            in vec3 Normal;

            // Color base desde el .mtl
            uniform vec3 baseColor;

            // Luces
            uniform int numLights;
            uniform vec3 lightPositions[16];
            uniform vec3 lightColors[16];
            uniform vec3 camPos;
            uniform float time;

            out vec3 outputColor;

            void main(){
                vec3 V = normalize(camPos - FragPos);
                vec3 N = normalize(Normal);

                vec3 ambient = baseColor * 0.02;
                vec3 result = ambient;

                float specularPower = 32.0;
                float specularStrength = 0.08;

                float light_range = 6.0;
                float light_soft = 2.0;

                for(int i = 0; i < numLights; i++){
                    vec3 L = lightPositions[i] - FragPos;
                    float light_dist = length(L);
                    L = normalize(L);

                    float diffuse = max(dot(N,L), 0.0);

                    vec3 R = reflect(-L, N);
                    float specular = pow(max(dot(V, R), 0.0), specularPower) * specularStrength;

                    float light_cutoff = 1.0 - smoothstep(light_range - light_soft, light_range, light_dist);
                    float light_attenuation = light_cutoff / (1.0 + 0.2 * light_dist + 0.5 * light_dist * light_dist);

                    float intensity = 1.0;
                    if (i > 0) {
                        float seed = fract(i * 12.9898 + i * 78.233 - i * 45.164);
                        float phase = seed * 6.2831853;
                        float f1 = 1.1 + seed * 0.9;
                        float f2 = 2.7 + seed * 1.3;
                        float f3 = 4.5 + seed * 2.1;

                        intensity = 0.8
                            + 0.25 * sin(time * f1 + phase)
                            + 0.10 * sin(time * f2 + phase * 1.7)
                            + 0.05 * sin(time * f3 + phase * 2.3);

                        intensity = clamp(intensity, 0.5, 1.2); 
                    }

                    vec3 contribution = (baseColor * diffuse + vec3(specular)) * lightColors[i] * light_attenuation;
                    result += contribution * intensity;
                }

                outputColor = result;
            }
        );
    }

    void init() {
        // Compilar shaders propios para el enemigo
        GLuint VertexDoorID = compilar_shader(vertex_enemy, GL_VERTEX_SHADER);
        GLuint FragmentDoorID = compilar_shader(fragment_enemy, GL_FRAGMENT_SHADER);

        prog_enemy = glCreateProgram();
        glAttachShader(prog_enemy, VertexDoorID);
        glAttachShader(prog_enemy, FragmentDoorID);
        glLinkProgram(prog_enemy);
        check_errores_programa(prog_enemy);

        glDetachShader(prog_enemy, VertexDoorID);
        glDeleteShader(VertexDoorID);
        glDetachShader(prog_enemy, FragmentDoorID);
        glDeleteShader(FragmentDoorID);

        // Cambiar al programa de enemigo
        glUseProgram(prog_enemy);

        enemy_model = cargar_modelo_assimp("bin/data/enemy/enemy.obj");
    }

    void shutdown() {
        // Borrar programa
        if (prog_enemy != 0) {
            glDeleteProgram(prog_enemy);
            prog_enemy = 0;
        }

        // Borrar modelo
        if (enemy_model.objs || enemy_model.mats || enemy_model.instIdx || enemy_model.buffers) {
            limpiar_escena(&enemy_model);
            enemy_model = {};
        }
    }

    void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V, vec3 cam_pos) {
        glUseProgram(prog_enemy);

        mat4 M = translate(mat4(1.0f), pos)
            * rotate(mat4(1.0f), rot_y, vec3(0, 1, 0))
            * glm::scale(mat4(1.0f), vec3(scale, scale, scale));

        transfer_mat4 ("MVP", P * V * M);
        transfer_mat4 ("M", M);
        transfer_vec3 ("camPos", cam_pos);
        transfer_float("time", (float)glfwGetTime());

        lighting::upload_to_shader(prog_enemy);
        glUseProgram(prog_enemy);

        for (unsigned int i = 0; i < enemy_model.nInstancias; i++){
            unsigned int j = enemy_model.instIdx[i];
            objeto& obj = enemy_model.objs[j];

            vec3 color = vec3(0.8f, 0.2f, 0.2f);
            if (enemy_model.diffuse) {
                color = enemy_model.diffuse[j];
            }
            transfer_vec3("baseColor", color);

            glBindVertexArray(obj.VAO);
            glDrawElements(GL_TRIANGLES, obj.Ni, obj.tipo_indice, (void*)0);
        }
        glBindVertexArray(0);
    }

    /**
     * Calcula la posición 3D de un enemigo a partir de su posición 2D en el mapa
     */
    vec3 compute_world_pos(int x, int y,
                        float tile_size, float maze_center_xz) {
        float wall_offset = tile_size * 0.5f; // Centro de celda
        float height = tile_size * HEIGHT_FACTOR; // Altura sobre el suelo

        float cx = x * tile_size - maze_center_xz;
        float cz = y * tile_size - maze_center_xz;

        return vec3(cx, height, cz);
    }
}
