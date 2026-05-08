#pragma once

#include "particle.h"
#include "particle_system.h"

/**
 * Estructura para representar un emisor de partículas.
 * Se encarga de generar nuevas partículas a lo largo del tiempo y añadirlas al sistema de partículas.
 */
struct ParticleEmitter {
    // Tipo de partícula a generar (fuego, humo, etc.)
    ParticleType typeToEmit;

    // Cuántas partículas por segundo generar
    float spawnRate = 0.0f;

    // Temporizador interno para controlar el spawn
    float spawnTimer = 0.0f;

    /**
     * Función de actualización del emisor, que se llama cada frame para generar nuevas partículas según el spawnRate
     * @param deltaTime tiempo transcurrido desde la última actualización (en segundos)
     * @param currentPos posición del emisor en el mundo 3D
     * @param pSystem referencia al sistema de partículas para emitir las nuevas partículas generadas
     */
    void update(float deltaTime, vec3 currentPos, ParticleSystem& pSystem);
};
