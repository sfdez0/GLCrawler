#include "GpO.h"
#include "portal.h"

namespace portal {
    namespace {
        GLuint prog_portal = 0;
        GLuint portal_VAO = 0;
        GLuint portal_VBO = 0;

        #define GLSL(src) "#version 330 core\n" #src

        const char* vertex_portal = GLSL(
            layout(location = 0) in vec2 pos;
            layout(location = 1) in vec2 uv;

            out vec2 UV;

            uniform mat4 MVP;

            void main() {
                gl_Position = MVP * vec4(pos.x, pos.y, 0.0, 1.0);
                UV = uv;
            }
        );

        const char* fragment_portal = GLSL(
        in vec2 UV;
        out vec4 outputColor;

        uniform float time;
        uniform float appear_t;

        float hash(vec2 p) {
            return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
        }
        float noise(vec2 p) {
            vec2 i = floor(p);
            vec2 f = fract(p);
            f = f * f * (3.0 - 2.0 * f);
            float a = hash(i);
            float b = hash(i + vec2(1.0, 0.0));
            float c = hash(i + vec2(0.0, 1.0));
            float d = hash(i + vec2(1.0, 1.0));
            return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
        }

        void main() {
            // Coordenadas centradas
            vec2 p = UV * 2.0 - 1.0;

            if (abs(p.x) > appear_t || abs(p.y) > appear_t) discard;

            // Coordenadas polares para el remolino
            float r = length(p);
            float angle = atan(p.y, p.x);

            // Dos espirales que rotan en sentidos opuestos
            float swirl1 = sin(angle * 5.0 + time * 1.8 - r * 6.0);
            float swirl2 = sin(angle * 3.0 - time * 1.2 + r * 4.0);
            float swirl  = (swirl1 + swirl2) * 0.5;

            // Ruido en coordenadas polares 
            vec2 polarUV = vec2(angle / 6.2831853 + time * 0.06,
                                r * 1.5 - time * 0.25);
            float n = noise(polarUV * 8.0);

            // Paleta oscura
            vec3 colorBlack  = vec3(0.01, 0.00, 0.03);
            vec3 colorPurple = vec3(0.18, 0.05, 0.40);
            vec3 colorBlue   = vec3(0.35, 0.25, 0.90);

            // Mezcla guiada por el swirl + ruido
            float t1 = swirl * 0.5 + 0.5;
            float t2 = clamp(t1 + (n - 0.5) * 0.4, 0.0, 1.0);
            vec3 col = mix(colorBlack, colorPurple, t2);
            col = mix(col, colorBlue, smoothstep(0.65, 1.0, t2));

            float vignette = 1.0 - smoothstep(0.5, 1.4, r);
            col *= mix(0.25, 1.0, vignette);

            // Núcleo brillante con pulso
            float pulse = 0.85 + 0.15 * sin(time * 2.5);
            float core  = exp(-r * 2.5) * 0.45 * pulse;
            col += colorBlue * core;

            // alpha = 1
            outputColor = vec4(col, 1.0);
        }
    );

    void crear_quad() {
        // Quad centrado en el origen, en plano XY local
        GLfloat quad_data[] = {
            -0.5f, -0.5f, 0.0f, 0.0f,
                0.5f, -0.5f, 1.0f, 0.0f,
                0.5f, 0.5f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 0.0f,
                0.5f, 0.5f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.0f, 1.0f
        };

        glGenBuffers(1, &portal_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, portal_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

        glGenVertexArrays(1, &portal_VAO);
        glBindVertexArray(portal_VAO);

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
        GLuint VertexID = compilar_shader(vertex_portal, GL_VERTEX_SHADER);
        GLuint FragmentID = compilar_shader(fragment_portal, GL_FRAGMENT_SHADER);
        prog_portal = glCreateProgram();
        glAttachShader(prog_portal, VertexID);
        glAttachShader(prog_portal, FragmentID);
        glLinkProgram(prog_portal);
        check_errores_programa(prog_portal);
        glDetachShader(prog_portal, VertexID);
        glDeleteShader(VertexID);
        glDetachShader(prog_portal, FragmentID);
        glDeleteShader(FragmentID);

        crear_quad();
    }

    void shutdown() {
        if (portal_VBO != 0) {
            glDeleteBuffers(1, &portal_VBO);
            portal_VBO = 0;
        }
        if (portal_VAO != 0) {
            glDeleteVertexArrays(1, &portal_VAO);
            portal_VAO = 0;
        }
        if (prog_portal != 0) {
            glDeleteProgram(prog_portal);
            prog_portal = 0;
        }
    }

    void draw(vec3 pos, float rot_y, float width, float height,
          float appear_t, float time,
          mat4 P, mat4 V) {
    glUseProgram(prog_portal);

    mat4 M = translate(mat4(1.0f), pos)
           * rotate(mat4(1.0f),
                    rot_y + glm::pi<float>() * 0.5f,
                    vec3(0, 1, 0))
           * glm::scale(mat4(1.0f), vec3(width, height, 1.0f));

    transfer_mat4("MVP", P * V * M);
    transfer_float("time", time);
    transfer_float("appear_t", appear_t);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);

    glBindVertexArray(portal_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

vec3 compute_portal_center(vec3 door_pos) {
    return vec3(door_pos.x+0.2f, door_pos.y -0.2f, door_pos.z);
}
}