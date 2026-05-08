#pragma once

#include <GpO.h>

namespace lighting {
    // Limita el array a este tamaño
    constexpr int MAX_LIGHTS = 16;

    // Vaciar el array de luces
    void clear();

    // Añadir una luz al array 
    void add(vec3 pos, vec3 color, float time);

    // Subir el array de luces al programa shader dado
    void upload_to_shader(GLuint prog);

    // Calcular la posición de la luz de una antorcha de pared
    vec3 compute_torch_light_pos(int x, int y, char direction,
                                 float tile_size, float maze_center_xz);
}