#include "lighting.h"
#include <vector>

namespace lighting {

namespace {
    std::vector<vec3> positions;
    std::vector<vec3> colors;
    std::vector<float> intensities;

    // Configuración de la posición de la luz 
    constexpr float WALL_OFFSET = -0.3f;
    constexpr float HEIGHT_FACTOR = 1.1f; 
}

void clear() {
    positions.clear();
    colors.clear();
    intensities.clear();
}

void add(vec3 pos, vec3 color, float time) {
    if ((int)positions.size() >= MAX_LIGHTS) return;
    positions.push_back(pos);
    colors.push_back(color);

    float intensity = 1.0;
    int i = (int)positions.size() - 1;
    if (i > 0) { // Excepto luz del personaje (siempre entra como primera luz)
        float seed = fract((float)i * 12.9898f + (float)i * 78.233f - (float)i * 45.164f); // Semilla basada en índice
        float phase = seed * 6.2831853f; // Fase "base"
        float f1 = 1.1f + seed * 0.9f; // Frecuencia 1
        float f2 = 2.7f + seed * 1.3f; // Frecuencia 2
        float f3 = 4.5f + seed * 2.1f; // Frecuencia 3

        // Aplicamos a la intensidad la suma de 3 ondas sinusoidales pseudoaleatorias
        intensity = 0.8f
            + 0.25f * sin(time * f1 + phase)
            + 0.10f * sin(time * f2 + phase * 1.7f)
            + 0.05f * sin(time * f3 + phase * 2.3f);

        // Limitamos intensidad entre 0.5 y 1.2
        intensity = clamp(intensity, 0.5f, 1.2f); 
    }

    intensities.push_back(intensity);
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
        glUniform1fv(glGetUniformLocation(prog, "lightIntensities"), n, (float*)intensities.data());
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