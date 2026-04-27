#pragma once

#include <GpO.h>

namespace torch_module {
    // Inicializar
    void init();

    // Dibujar una antorcha en la posición y orientación dadas
    void draw(vec3 pos, float scale, float rot_y, mat4 P, mat4 V);

    // Calcular la posición de una antorcha de pared
    vec3 compute_world_pos(int x, int y, char direction,
                           float tile_size, float maze_center_xz);

    // Calcular la rotación según la dirección de la pared
    float compute_rotation(char direction);
}