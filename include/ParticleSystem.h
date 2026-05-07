#pragma once

#include "Particle.h"
#include <vector>

/**
 * Clase para gestionar el sistema de partículas del juego. Se encarga de almacenar todas las partículas activas, actualizarlas cada frame y renderizarlas.
 */
class ParticleSystem {
private:
    // Número máximo de partículas
    const int maxParticles = 4096;

    float global_time = 0.0f;

    size_t head = 0;

public:
    /**
     * Función para inicializar el sistema de partículas
     */
    void init();

    /**
     * Libera recursos GPU del sistema de partículas
     */
    void shutdown();
    
    /**
     * Función para emitir una nueva partícula
     * @param newParticle partícula a emitir
     */
    void emit(Particle newParticle);

    /**
     * Función de actualización de todas las partículas
     * @param deltaTime tiempo transcurrido desde la última actualización (en segundos)
     */
    void update(float deltaTime);

    /**
     * Función para subir los datos de las partículas al buffer
     */
    void upload_particles();

    /**
     * Función para renderizar las partículas en pantalla
     * @param P matriz de proyección
     * @param V matriz de vista
     */
    void render(mat4 P, mat4 V);

    // Posición del icono de llaves
    glm::vec2 pickup_target_ndc = glm::vec2(0.81f, 0.90f);
};
