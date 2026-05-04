#pragma once

#include <GpO.h>

namespace flame {
    void init();

    // Liberar recursos GPU
    void shutdown();

    // Dibujar una llama animada (billboard) en la posición dada
    void draw(vec3 pos, float scale, mat4 P, mat4 V);

    // Calcular la posición de la llama a partir de la posición del torch y la dirección de la pared a la que está pegado
    vec3 compute_position(vec3 torch_pos, char direction);
}