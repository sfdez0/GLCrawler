#pragma once
#include "GpO.h"

namespace damage_overlay {
    void init();
    void shutdown();

    // Llamar cada vez que el jugador recibe un golpe
    void trigger(float intensity = 1.0f);

    // Actualiza el desvanecimiento 
    void update(float deltaTime);

    // Dibuja el overlay 
    void draw();
}