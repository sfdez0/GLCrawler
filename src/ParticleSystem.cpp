#include "ParticleSystem.h"

// Programa y buffers
GLuint prog_particles = 0;
GLuint vao_particles = 0;
GLuint vbo_particles = 0;

#define GLSL(src) "#version 330 core\n" #src

const char* vertex_particles = GLSL(
    layout(location = 0) in vec3 initial_pos;
    layout(location = 1) in vec3 initial_vel;
    layout(location = 2) in vec4 color;
    layout(location = 3) in float size;
    layout(location = 4) in float birth_time;
    layout(location = 5) in float max_life;
    layout(location = 6) in float type;

    out vec4 vColor;
    out float vAlphaModifier; // Para hacer transparente la partícula según su edad

    uniform mat4 VP;
    uniform float current_time;
    uniform vec3 gravity;

    void main() {
        // Calculamos la edad de la partícula
        float age = current_time - birth_time;

        // Si la particula ha muerto, la mandamos lejos para que sea descartada por Culling
        if (age < 0.0 || age > max_life) {
            gl_Position = vec4(-1000.0, -1000.0, -1000.0, 1.0);
            gl_PointSize = 0.0;
            return;
        }

        switch (int(type)) {
            case 0: // Fuego
                // Calculamos la posición actual de la partícula usando cinemática (pos = pos0 + vel0 * t + 1/2 * a * t^2)
                vec3 current_pos = initial_pos + (initial_vel * age) + (0.5 * gravity * age * age);

                // Calculamos la reducción de tamaño de la partícula
                gl_Position = VP * vec4(current_pos, 1.0);
                gl_PointSize = size / gl_Position.w;
                break;
            case 1: // Recogida de Llave
                // Gravedad casi nula con leve flotación hacia arriba
                vec3 pickup_gravity = vec3(0.0, 0.4, 0.0);

                // Aplicamos un amortiguamiento exponencial para que frenan rápido
                float damp = exp(-1.2 * age);
                current_pos = initial_pos + (initial_vel * age * damp) 
                                        + (0.5 * pickup_gravity * age * age);

                gl_Position = VP * vec4(current_pos, 1.0);

                // Reducimos el tamaño progresivamente
                float shrink = 1.0 - (age / max_life) * 0.85;
                gl_PointSize = size * shrink / gl_Position.w;
                break;
            case 2: // Llave
                // Gravedad casi nula con leve flotación hacia arriba
                vec3 key_gravity = vec3(0.0, 0.0, 0.0);
                current_pos = initial_pos + initial_vel * age 
                            + 0.5 * key_gravity * age * age;

                gl_Position = VP * vec4(current_pos, 1.0);

                // Reduccimos el de tamaño progresivamente + parpadeo
                float key_shrink = 1.0 - (age / max_life) * 0.6;
                float twinkle = 0.85 + 0.15 * sin(age * 12.0 + initial_pos.x * 5.0);
                gl_PointSize = size * key_shrink * twinkle / gl_Position.w;
                break;
            default:
                break;
        }

        // Pasamos el color
        vColor = color;

        // Calculamos el alpha para difuminar la partícula al envejecer
        vAlphaModifier = 1.0 - (age / max_life);
    }
);

const char* fragment_particles = GLSL(
    in vec4 vColor;
    in float vAlphaModifier;
    out vec4 outputColor;

    void main() {
        vec2 p = gl_PointCoord * 2.0 - 1.0;
        float d = length(p);
        float circle_alpha = 1.0 - smoothstep(0.7, 1.0, d);
        if (circle_alpha <= 0.01) discard;
        
        // Multiplicamos el alpha del color por el factor alpha de la partícula
        outputColor = vec4(vColor.rgb, vColor.a * circle_alpha * vAlphaModifier);
    }
);

// Gravedad para partículas de fuego
const vec3 fire_gravity = vec3(0.0f, -0.45f, 0.0f);

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

    // Tamaño de cada particula (pos(3) + vel(3) + color(4) + size(1) + birth(1) + max_life(1) + type(1))
    const int size = 14 * sizeof(float);
    
    // Reservamos espacio para el límite máximo de partículas (vacío al principio)
    glBufferData(GL_ARRAY_BUFFER, maxParticles * size, nullptr, GL_DYNAMIC_DRAW);

    // initial_pos (0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, size, (void*)0);
    // initial_vel (1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, size, (void*)(3 * sizeof(float)));
    // color (2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, size, (void*)(6 * sizeof(float)));
    // size (3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, size, (void*)(10 * sizeof(float)));
    // birth_time (4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, size, (void*)(11 * sizeof(float)));
    // max_life (5)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, size, (void*)(12 * sizeof(float)));
    // type (6)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, size, (void*)(13 * sizeof(float)));

    glBindVertexArray(0);
}

/**
 * Función para liberar recursos GPU del sistema de partículas
 */
void ParticleSystem::shutdown() {
    if (prog_particles != 0) {
        glDeleteProgram(prog_particles);
        prog_particles = 0;
    }

    if (vbo_particles != 0) {
        glDeleteBuffers(1, &vbo_particles);
        vbo_particles = 0;
    }

    if (vao_particles != 0) {
        glDeleteVertexArrays(1, &vao_particles);
        vao_particles = 0;
    }

    global_time = 0.0f;
    head = 0;
}

/**
 * Función para emitir una nueva partícula
 * @param newParticle partícula a emitir
 */
void ParticleSystem::emit(Particle newParticle) {
    // Creamos el paquete de 14 floats de esta única partícula
    float data[14] = {
        newParticle.pos.x, newParticle.pos.y, newParticle.pos.z,
        newParticle.vel.x, newParticle.vel.y, newParticle.vel.z,
        newParticle.color.r, newParticle.color.g, newParticle.color.b, newParticle.color.a,
        newParticle.size,
        global_time, // Hora de spawn
        newParticle.max_life,
        (float)newParticle.type
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo_particles);
    
    // Calculamos dónde escribir usando head como índice
    GLintptr offset = head * 14 * sizeof(float);
    
    // Subimos los datos de la partícula
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(data), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Movemos el índice (si llega al máximo vuelve al inicio, sobreescribiendo partículas viejas)
    head = (head + 1) % maxParticles;
}

/**
 * Función de actualización de todas las partículas
 * @param deltaTime tiempo transcurrido desde la última actualización (en segundos)
 */
void ParticleSystem::update(float deltaTime) {
    global_time += deltaTime;
}

/**
 * Función para renderizar las partículas en pantalla
 * @param P matriz de proyección
 * @param V matriz de vista
 */
void ParticleSystem::render(mat4 P, mat4 V) {
    glUseProgram(prog_particles);

    // Enviamos VP, tiempo y gravedad
    transfer_mat4("VP", P * V);
    transfer_float("current_time", global_time);
    transfer_vec3("gravity", fire_gravity);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glEnable(GL_PROGRAM_POINT_SIZE);

    glBindVertexArray(vao_particles);
    // Mandamos a dibujar el buffer COMPLETO siempre.
    // El shader se encargará de esconder las que estén muertas.
    glDrawArrays(GL_POINTS, 0, maxParticles);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDisable(GL_BLEND);
}
