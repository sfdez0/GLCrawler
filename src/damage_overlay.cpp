#include "GpO.h"
#include "damage_overlay.h"

namespace damage_overlay {
    namespace {
        GLuint prog_overlay = 0;
        GLuint overlay_VAO = 0;
        GLuint overlay_VBO = 0;

        // Estado de la animación
        float current_alpha = 0.0f; // Alpha actual del overlay
        constexpr float FADE_SPEED = 1.8f; // Velocidad de desvanecimiento (1/seg)
        constexpr float MAX_ALPHA = 0.6f; // Alpha máxima al recibir daño

        #define GLSL(src) "#version 330 core\n" #src

        const char* vertex_overlay = GLSL(
            layout(location = 0) in vec2 pos;
            layout(location = 1) in vec2 uv;

            out vec2 UV;

            void main() {
                gl_Position = vec4(pos, 0.0, 1.0);
                UV = uv;
            }
        );

        const char* fragment_overlay = GLSL(
            in vec2 UV;
            out vec4 outputColor;

            uniform float alpha;

            void main() {
                // Coordenadas centradas
                vec2 p = UV * 2.0 - 1.0;
                float dist = length(p);

                // Muy poco rojo en el centro, fuerte en los bordes
                float vignette = smoothstep(0.15, 1.1, dist);

                // Color rojo, oscuro hacia los bordes
                vec3 redColor = mix(vec3(1.0, 0.15, 0.05),
                                    vec3(0.45, 0.0, 0.0),
                                    vignette);

                // el centro queda casi limpio, los bordes muy rojos
                float finalAlpha = alpha * (0.25 + 0.75 * vignette);

                outputColor = vec4(redColor, finalAlpha);
            }
        );

        void crear_quad() {
            // Quad de pantalla completa en NDC (-1..1)
            GLfloat quad_data[] = {
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,
                 1.0f,  1.0f,  1.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f,  1.0f,  1.0f, 1.0f,
                -1.0f,  1.0f,  0.0f, 1.0f
            };

            glGenBuffers(1, &overlay_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, overlay_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

            glGenVertexArrays(1, &overlay_VAO);
            glBindVertexArray(overlay_VAO);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                                  (void*)(2 * sizeof(float)));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }

    void init() {
        GLuint VertexID = compilar_shader(vertex_overlay, GL_VERTEX_SHADER);
        GLuint FragmentID = compilar_shader(fragment_overlay, GL_FRAGMENT_SHADER);

        prog_overlay = glCreateProgram();
        glAttachShader(prog_overlay, VertexID);
        glAttachShader(prog_overlay, FragmentID);
        glLinkProgram(prog_overlay);
        check_errores_programa(prog_overlay);

        glDetachShader(prog_overlay, VertexID);
        glDeleteShader(VertexID);
        glDetachShader(prog_overlay, FragmentID);
        glDeleteShader(FragmentID);

        crear_quad();
    }

    void shutdown() {
        if (overlay_VBO != 0) {
            glDeleteBuffers(1, &overlay_VBO);
            overlay_VBO = 0;
        }
        if (overlay_VAO != 0) {
            glDeleteVertexArrays(1, &overlay_VAO);
            overlay_VAO = 0;
        }
        if (prog_overlay != 0) {
            glDeleteProgram(prog_overlay);
            prog_overlay = 0;
        }
        current_alpha = 0.0f;
    }

    void trigger(float intensity) {
        // Coge el máximo entre el alpha actual y el nuevo
        float new_alpha = MAX_ALPHA * glm::clamp(intensity, 0.0f, 1.0f);
        if (new_alpha > current_alpha) current_alpha = new_alpha;
    }

    void update(float deltaTime) {
        // Decae linealmente el alpha hasta cero
        if (current_alpha > 0.0f) {
            current_alpha -= FADE_SPEED * deltaTime;
            if (current_alpha < 0.0f) current_alpha = 0.0f;
        }
    }

    void draw() {
        // Si no hay efecto activo, ahorramos draw call
        if (current_alpha <= 0.001f) return;

        glUseProgram(prog_overlay);
        transfer_float("alpha", current_alpha);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        glBindVertexArray(overlay_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}