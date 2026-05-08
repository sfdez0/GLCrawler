#pragma once

#define PORTAL_H

#include "GpO.h"

namespace portal {
    // Inicializa el shader y los buffers del portal.
    void init();
    
    //Libera todos los recursos del portal
    void shutdown();
    
    // Dibujar una portal en la posición indicada
    void draw(vec3 pos, float rot_y, float width, float height,
          float appear_t, float time, mat4 P, mat4 V);
          
    // Devuelve el centro del portal a la altura de la cámara
    vec3 compute_portal_center(vec3 door_pos);
}