#pragma once

#include <GpO.h>

namespace enemy {
    // Inicializar
    void init();

    // Liberar recursos GPU
    void shutdown();

    // Dibujar enemigo en la posición y orientación dadas
    void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V, vec3 cam_pos);

    // Calcular la posición de un enemigo
    vec3 compute_world_pos(int x, int y, float tile_size, float maze_center_xz);
}
