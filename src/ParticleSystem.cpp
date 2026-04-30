#include "ParticleSystem.h"

// Programa y buffers
GLuint prog_particles = 0;
GLuint vao_particles = 0;
GLuint vbo_particles = 0;

#define GLSL(src) "#version 330 core\n" #src

const char* vertex_particles = GLSL(
    layout(location = 0) in vec3 pos;
    layout(location = 1) in vec4 color;
    layout(location = 2) in float size;

    out vec4 vColor;

    uniform mat4 VP;

    void main() {
        gl_Position = VP * vec4(pos, 1.0);

        // Ajustamos tamaño según la distancia a la cámara
        float new_size = size / gl_Position.w;

        gl_PointSize = new_size;
        vColor = color;
    }
);

const char* fragment_particles = GLSL(
    in vec4 vColor;
    out vec4 outputColor;

    void main() {
        vec2 p = gl_PointCoord * 2.0 - 1.0;
        float d = length(p);
        float alpha = 1.0 - smoothstep(0.7, 1.0, d);
        if (alpha <= 0.01) discard;
        outputColor = vec4(vColor.rgb, vColor.a * alpha);
    }
);

// Gravedad para partículas de fuego
const vec3 fire_gravity = vec3(0.0f, -0.45f, 0.0f);

/**
 * Función que devuelve el número actual de partículas activas en el sistema
 * @return número de partículas activas
 */
size_t ParticleSystem::get_free_particles() const {
    return maxParticles - particles.size();
}

/**
 * Función para inicializar el sistema de partículas
 */
void ParticleSystem::init() {
    // Compilamos shaders
    GLuint VertexID = compilar_shader(vertex_particles, GL_VERTEX_SHADER);
    GLuint FragmentID = compilar_shader(fragment_particles, GL_FRAGMENT_SHADER);

    // Creamos programa en GPU y lo vinculamos con los shaders
    prog_particles = glCreateProgram();
    glAttachShader(prog_particles, VertexID);
    glAttachShader(prog_particles, FragmentID);
    glLinkProgram(prog_particles);
    check_errores_programa(prog_particles);

    // Limpiamos shaders
    glDetachShader(prog_particles, VertexID);
    glDeleteShader(VertexID);
    glDetachShader(prog_particles, FragmentID);
    glDeleteShader(FragmentID);

    // Configuramos VAO y VBO para las partículas
    glGenVertexArrays(1, &vao_particles);
    glBindVertexArray(vao_particles);

    glGenBuffers(1, &vbo_particles);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_particles);

    // Espacio para el máximo número de partículas, cada una con 8 floats (pos(3) + color(4) + size(1))
    const int size = 8;
    glBufferData(GL_ARRAY_BUFFER, maxParticles * size * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, size * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, size * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, size * sizeof(float), (void*)((3 + 4) * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * Función para emitir una nueva partícula
 * @param newParticle partícula a emitir
 */
void ParticleSystem::emit(Particle newParticle) {
    particles.push_back(newParticle);
}

/**
 * Función de actualización de todas las partículas
 * @param deltaTime tiempo transcurrido desde la última actualización (en segundos)
 */
void ParticleSystem::update(float deltaTime) {
    // Iteramos por todas las partículas del sistema
    for (auto& p : particles) {
        // Reducimos la vida de la partícula
        p.life -= deltaTime;
        
        // Aplicamos diferentes comportamientos según el tipo de partícula
        switch (p.type) {
            case ParticleType::Fire:
                p.vel += fire_gravity * deltaTime;
                p.pos += p.vel * deltaTime;
                break;
            default:
                break;
        }
    }

    // Eliminamos las partículas que han muerto
    particles.erase(
        // Posición de inicio de borrado (todas las partículas vivas están antes)
        std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0.0f; }),

        // Posición de fin de borrado (todas las partículas muertas están después)
        particles.end()
    );
}

/**
 * Función para subir los datos de las partículas al buffer
 */
void ParticleSystem::upload_particles() {
    const int size = 8; // pos(3) + color(4) + size(1)
    std::vector<float> data;
    data.reserve(particles.size() * size);

    // Por cada particula, añadimos su posición, color y tamaño al vector de datos
    for (const Particle& p : particles) {
        float alpha = p.life / p.max_life;
        data.push_back(p.pos.x);
        data.push_back(p.pos.y);
        data.push_back(p.pos.z);
        data.push_back(p.color.r);
        data.push_back(p.color.g);
        data.push_back(p.color.b);
        data.push_back(p.color.a * alpha); // Alpha modulado por la vida restante
        data.push_back(p.size);
    }

    // Subimos los datos al buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo_particles);
    if (!data.empty()) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/**
 * Función para renderizar las partículas en pantalla
 * @param P matriz de proyección
 * @param V matriz de vista
 */
void ParticleSystem::render(mat4 P, mat4 V) {
    if (particles.empty()) {
        return;
    }

    // Subimos los datos de partículas al buffer
    upload_particles();

    // Dibujamos las partículas
    glUseProgram(prog_particles);
    transfer_mat4("VP", P * V);

    // Blend para transparencia y sin depth buffer
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(vao_particles);
    glDrawArrays(GL_POINTS, 0, (GLsizei)particles.size());
    glBindVertexArray(0);

    // Restauramos estado
    glDepthMask(GL_TRUE);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
}
