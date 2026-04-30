#pragma once

#include "Particle.h"
#include <vector>

/**
 * Clase para gestionar el sistema de partículas del juego. Se encarga de almacenar todas las partículas activas, actualizarlas cada frame y renderizarlas.
 */
class ParticleSystem {
private:
    // Lista de partículas activas en el sistema
    std::vector<Particle> particles;

    // Número máximo de partículas
    const int maxParticles = 1024;

public:
    /**
     * Función que devuelve el número actual de partículas activas en el sistema
     * @return número de partículas activas
     */
    size_t get_free_particles() const;

    /**
     * Función para inicializar el sistema de partículas
     */
    void init();
    
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
};
