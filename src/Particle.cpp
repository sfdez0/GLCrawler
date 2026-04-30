#include "Particle.h"

// Función para generar un número aleatorio entre a y b
float randf(float a, float b) {
    return a + (b - a) * (float(rand()) / float(RAND_MAX));
}

/**
 * Función para crear una partícula de fuego
 */
Particle Particle::CreateFire(vec3 pos){
    Particle p;
    p.pos = pos;
    p.vel = vec3(randf(-0.25f, 0.25f), randf(0.3f, 1.0f), randf(-0.25f, 0.25f)); // Velocidad aleatoria para simular movimiento de partículas de fuego
    p.max_life = randf(1.2f, 2.0f); // Vida aleatoria
    p.life = p.max_life;
    p.size = randf(10.0f, 20.0f); // Tamaño aleatorio
    p.color = vec4(1.0f, randf(0.5f, 0.8f), 0.2f, 1.0f); // Color base naranja, con componente verde aleatorio
    p.type = ParticleType::Fire;
    return p;
}
