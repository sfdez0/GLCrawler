#include "ParticleEmitter.h"

/**
 * Función de actualización del emisor, que se llama cada frame para generar nuevas partículas según el spawnRate
 * @param deltaTime tiempo transcurrido desde la última actualización (en segundos)
 * @param currentPos posición del emisor en el mundo 3D
 * @param pSystem referencia al sistema de partículas para emitir las nuevas partículas generadas
 */
void ParticleEmitter::update(float deltaTime, vec3 currentPos, ParticleSystem& pSystem) {
    if (spawnRate <= 0.0f) {
        return;
    }

    // Actualizamos el temporizador con el tiempo transcurrido
    spawnTimer += deltaTime;
    
    // Si toca spawnear
    while (spawnTimer > (1.0f / spawnRate)) {
        spawnTimer -= (1.0f / spawnRate);
        
        // Según el tipo de partícula, la creamos y pedimos al sistema emitirla
        switch (typeToEmit) {
            case ParticleType::Fire:
                pSystem.emit(Particle::CreateFire(currentPos));
                break;
            case ParticleType::Key:
                pSystem.emit(Particle::CreateKey(currentPos));
                break;
            default:
                break;
        }
    }
}
