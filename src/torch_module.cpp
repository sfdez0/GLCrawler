#include "torch_module.h"
#include <GPO_assimp_aux.h>

namespace torch_module {

namespace {
    GLuint prog_torch = 0;
    GLuint tex_torch = 0;
    struct escena torch_model;

    // Configuración del posicionamiento 
    constexpr float WALL_OFFSET = -0.4f;   // se añade a tile_size * 0.5f
    constexpr float HEIGHT_FACTOR = 1.0f;  // multiplica tile_size

    #define GLSL(src) "#version 330 core\n" #src

    // layout sigue el orden del cargador Assimp: pos = 0, normal = 1, uv = 2
    const char* vertex_torch = GLSL(
        layout(location = 0) in vec3 pos;
        layout(location = 1) in vec3 normal;
        layout(location = 2) in vec2 uv;

        out vec2 UV;

        uniform mat4 MVP;

        void main(){
            gl_Position = MVP * vec4(pos, 1);
            UV = uv;
        }
    );

    const char* fragment_torch = GLSL(
        in vec2 UV;
        uniform sampler2D tex;
        out vec3 outputColor;

        void main() {
            outputColor = texture(tex, UV).rgb;
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

	// Cargar la textura difusa de la antorcha
	tex_torch = cargar_textura_rgba("/bin/data/torch/torch_hanger_lp_torch_hanger_BaseColor.png", GL_TEXTURE2);
	transfer_int("tex", 2);

	// Cargar el modelo 3D usando Assimp
	torch_model = cargar_modelo_assimp("/bin/data/torch/torch.obj");
}

void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V) {
    glUseProgram(prog_torch);

    mat4 M = translate(mat4(1.0f), pos)
           * rotate(mat4(1.0f), rot_y, vec3(0, 1, 0))
           * glm::scale(mat4(1.0f), vec3(scale, scale, scale));

    transfer_mat4("MVP", P * V * M);

    for (unsigned int i = 0; i < torch_model.nInstancias; i++) {
        unsigned int j = torch_model.instIdx[i];
        objeto& obj = torch_model.objs[j];
        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.Ni, obj.tipo_indice, (void*)0);
    }
    glBindVertexArray(0);
}

vec3 compute_world_pos(int x, int y, char direction,
                      float tile_size, float maze_center_xz) {
    float wall_offset = tile_size * 0.5f + WALL_OFFSET;
    float height = tile_size * HEIGHT_FACTOR;

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
        case 'u': return -glm::pi<float>() * 0.5f;
        case 'd': return glm::pi<float>() * 0.5f;
        case 'l': return glm::pi<float>(); 
        case 'r': return 0.0f;
    }
    return 0.0f;
}

}