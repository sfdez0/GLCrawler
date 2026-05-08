#pragma once

#include <GpO.h>

namespace door {
    // Inicializar
    void init();

    // Liberar recursos GPU
    void shutdown();

    // Dibujar una puerta en la posición y orientación dadas
    void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V, vec3 cam_pos, float open_angle = 0.0f);

    // Calcular la posición de una puerta
    vec3 compute_world_pos(int x, int y, char direction,
                           float tile_size, float maze_center_xz);

    // Calcular la rotación según la dirección de la pared
    float compute_rotation(char direction);
}