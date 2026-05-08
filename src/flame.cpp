#include "GpO.h"
#include "flame.h"

namespace flame {
    namespace {
        GLuint prog_flame = 0;
        GLuint flame_VAO = 0;
        GLuint flame_VBO = 0;
        GLuint tex_flame = 0;

        #define GLSL(src) "#version 330 core\n" #src

        const char* vertex_flame = GLSL(
            layout(location = 0) in vec2 pos;
            layout(location = 1) in vec2 uv;

            out vec2 UV;

            uniform mat4 P;
            uniform mat4 V;
            uniform vec3 worldPos;
            uniform float scale;

            void main() {
                vec3 camRight = vec3(V[0][0], V[1][0], V[2][0]);
                vec3 camUp    = vec3(V[0][1], V[1][1], V[2][1]);

                vec3 worldVertex = worldPos
                                + camRight * pos.x * scale
                                + camUp    * pos.y * scale;

                gl_Position = P * V * vec4(worldVertex, 1.0);
                UV = uv;
            }
        );

        const char* fragment_flame = GLSL(
            in vec2 UV;
            out vec4 outputColor;

            uniform sampler2D flameTex;
            uniform int frameIdx;
            uniform int totalFrames;

            void main() {
                float frameWidth = 1.0 / float(totalFrames);
                vec2 finalUV = vec2(
                    UV.x * frameWidth + float(frameIdx) * frameWidth,
                    UV.y
                );

                vec4 col = texture(flameTex, finalUV);

                if (col.a < 0.1) discard;

                outputColor = col;
            }
        );

        void crear_quad() {
            GLfloat quad_data[] = {
                -0.5f, -0.5f, 0.0f, 0.0f,
                0.5f, -0.5f, 1.0f, 0.0f,
                0.5f,  0.5f, 1.0f, 1.0f,
                -0.5f, -0.5f, 0.0f, 0.0f,
                0.5f,  0.5f, 1.0f, 1.0f,
                -0.5f,  0.5f, 0.0f, 1.0f
            };

            glGenBuffers(1, &flame_VBO);
            glBindBuffer(GL_ARRAY_BUFFER, flame_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);

            glGenVertexArrays(1, &flame_VAO);
            glBindVertexArray(flame_VAO);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    } 

    void init() {
        GLuint VertexFlameID = compilar_shader(vertex_flame, GL_VERTEX_SHADER);
        GLuint FragmentFlameID = compilar_shader(fragment_flame, GL_FRAGMENT_SHADER);
        prog_flame = glCreateProgram();
        glAttachShader(prog_flame, VertexFlameID);
        glAttachShader(prog_flame, FragmentFlameID);
        glLinkProgram(prog_flame);
        check_errores_programa(prog_flame);
        glDetachShader(prog_flame, VertexFlameID);
        glDeleteShader(VertexFlameID);
        glDetachShader(prog_flame, FragmentFlameID);
        glDeleteShader(FragmentFlameID);

        glUseProgram(prog_flame);
        tex_flame = cargar_textura_rgba("bin/data/flame.png", GL_TEXTURE0);
        transfer_int("flameTex", 0);
        transfer_int("totalFrames", 4);

        crear_quad();
    }

    void shutdown() {
        if (flame_VBO != 0) {
            glDeleteBuffers(1, &flame_VBO);
            flame_VBO = 0;
        }

        if (flame_VAO != 0) {
            glDeleteVertexArrays(1, &flame_VAO);
            flame_VAO = 0;
        }

        if (tex_flame != 0) {
            glDeleteTextures(1, &tex_flame);
            tex_flame = 0;
        }

        if (prog_flame != 0) {
            glDeleteProgram(prog_flame);
            prog_flame = 0;
        }
    }

    void draw(vec3 pos, float scale, mat4 P, mat4 V) {
        glUseProgram(prog_flame);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_flame);

        float t = (float)glfwGetTime();
        int frameIdx = ((int)(t * 10.0f)) % 4;

        transfer_mat4("P", P);
        transfer_mat4("V", V);
        transfer_vec3("worldPos", pos);
        transfer_float("scale", scale);
        transfer_int("frameIdx", frameIdx);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glBindVertexArray(flame_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    vec3 compute_position(vec3 torch_pos, char direction) {
        float flame_forward = 0.05f;
        vec3 offset(0.0f, 0.25f, 0.0f);

        switch (direction) {
            case 'u': offset.z = +flame_forward; break;
            case 'd': offset.z = -flame_forward; break;
            case 'l': offset.x = +flame_forward; break;
            case 'r': offset.x = -flame_forward; break;
        }

        return torch_pos + offset;
    }
} 
