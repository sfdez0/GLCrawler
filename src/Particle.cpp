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

/**
 * Función para crear una partícula de destello para el efecto de recogida de llave
 */
Particle Particle::CreatePickup(vec3 pos){
    Particle p;
    p.pos = pos;

    // Generamos una dirección esférica aleatoria con ligero sesgo hacia arriba
    float theta = randf(0.0f, 6.2831853f); // ángulo horizontal
    float phi = randf(-0.3f, 1.2f); // ángulo vertical sesgado hacia arriba
    float speed = randf(1.5f, 3.5f); // velocidad inicial alta

    p.vel = vec3(
        cos(phi) * cos(theta) * speed,
        sin(phi) * speed,
        cos(phi) * sin(theta) * speed
    );

    p.max_life = 2.2f; // Vida fijo
    p.life = p.max_life;
    p.size = randf(6.0f, 12.0f); // Tamaño aleatorio

    // Color verde-dorado
    p.color = vec4(
        randf(0.6f, 1.0f), // R
        randf(0.85f, 1.0f), // G
        randf(0.2f, 0.5f), // B
        1.0f
    );
    p.type = ParticleType::Pickup;
    return p;
}

/**
 * Función para crear una partícula de aura que orbita alrededor de la llave
 */
Particle Particle::CreateKey(vec3 pos){
    Particle p;

    // Posición en un anillo alrededor del centro
    float angle = randf(0.0f, 6.2831853f);
    float ring_radius = randf(0.05f, 1.0f); // Anchura
    float vertical_offset = randf(-0.8f, 0.15f); // Altura

    p.pos = pos + vec3(
        cos(angle) * ring_radius,
        vertical_offset,
        sin(angle) * ring_radius
    );

    // Velocidad tangencial + leve subida
    float tangent_speed = randf(0.5f, 1.0f);
    float rise_speed = randf(0.6f, 1.0f);

    p.vel = vec3(
        -sin(angle) * tangent_speed,
         rise_speed,
         cos(angle) * tangent_speed
    );

    p.max_life = randf(1.2f, 1.8f);
    p.life = p.max_life;
    p.size = randf(4.0f, 9.0f);

    // Verde-dorado
    p.color = vec4(
        randf(0.5f, 0.9f),
        randf(0.85f, 1.0f),
        randf(0.2f, 0.5f),
        0.8f
    );
    p.type = ParticleType::Key;
    return p;
}