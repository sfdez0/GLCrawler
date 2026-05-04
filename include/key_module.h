#pragma once

#include <GpO.h>

namespace key_module {
    // Inicializar
    void init();

    // Dibujar una llave en la posición dada con animación de flotación + rotación
    void draw(vec3 base_pos, float scale, float time, mat4 P, mat4 V, vec3 cam_pos);

    // Calcular la posición 3D de una llave a partir de sus coordenadas 2D del mapa
    vec3 compute_world_pos(int x, int y, float tile_size, float maze_center_xz);

    // Calcular la posición de la luz dorada que acompaña a la llave
    vec3 compute_light_pos(vec3 base_pos);
}