#pragma once

#include <GpO.h>

/**
 * Enum para representar el tipo de partícula (fuego, humo, etc.)
 */
enum class ParticleType {
    Fire,
    Smoke,
    Pickup
};

/**
 * Estructura para representar una partícula individual
 */
struct Particle {
    // Posición actual de la partícula en el mundo 3D
    vec3 pos;
    // Velocidad actual de la partícula
    vec3 vel;
    // Color base de la partícula
    vec4 color;
    // Tiempo de vida restante de la partícula (en segundos)
    float life;
    // Tiempo total de vida de la partícula (en segundos)
    float max_life;
    // Tamaño actual de la partícula (en píxeles)
    float size;
    // Tipo de particula
    ParticleType type;

    /**
     * Función para crear una partícula de fuego
     */
    static Particle CreateFire(vec3 pos);

    /**
     * Función para crear una partícula de recoger llave
     */
    static Particle CreatePickup(vec3 pos);
};
