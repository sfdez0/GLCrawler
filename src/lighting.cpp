#include "lighting.h"
#include <vector>

namespace lighting {

namespace {
    std::vector<vec3> positions;
    std::vector<vec3> colors;

    // Configuración de la posición de la luz 
    constexpr float WALL_OFFSET = -0.3f;
    constexpr float HEIGHT_FACTOR = 1.1f; 
}

void clear() {
    positions.clear();
    colors.clear();
}

void add(vec3 pos, vec3 color) {
    if ((int)positions.size() >= MAX_LIGHTS) return;
    positions.push_back(pos);
    colors.push_back(color);
}

int count() {
    return (int)positions.size();
}

void upload_to_shader(GLuint prog) {
    glUseProgram(prog);
    int n = (int)positions.size();
    glUniform1i(glGetUniformLocation(prog, "numLights"), n);
    if (n > 0) {
        glUniform3fv(glGetUniformLocation(prog, "lightPositions"), n, (float*)positions.data());
        glUniform3fv(glGetUniformLocation(prog, "lightColors"),    n, (float*)colors.data());
    }
}

vec3 compute_torch_light_pos(int x, int y, char direction,
                             float tile_size, float maze_center_xz) {
    float wall_offset = tile_size * 0.5f - WALL_OFFSET;
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

}