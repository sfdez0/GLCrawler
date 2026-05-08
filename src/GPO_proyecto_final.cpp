/**
 * Proyecto final de GpO
 * Título: Dungeon Crawler 3D
 * ATG, 2019
 */

#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GpO.h>
#include <GPO_assimp_aux.h>

#include "maze.h"
#include "flame.h"
#include "lighting.h"
#include "torch_module.h"
#include "door.h"
#include "key_module.h"
#include "enemy.h"
#include "ParticleEmitter.h"
#include "pathfinding.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     VARIABLES 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* -- Variables OpenGL -- */
// Tamaño inicial ventana
int ANCHO = 800, ALTO = 600; 

// Título de la ventana
const char* prac = "OpenGL (GpO)"; 

GLFWwindow* window;
GLuint prog = 0;
// Texturas de paredes
GLuint tex_brick = 0;
GLuint tex_normal = 0;
GLuint tex_displacement = 0;
GLuint tex_ao = 0;
// Texturas de suelo
GLuint tex_floor = 0;
GLuint tex_floor_normal = 0;
GLuint tex_floor_displacement = 0;
GLuint tex_floor_ao = 0;
// Texturas de techo
GLuint tex_ceiling = 0;
GLuint tex_ceiling_normal = 0;
GLuint tex_ceiling_displacement = 0;
GLuint tex_ceiling_ao = 0;
objeto escena_cubica;
GLuint escena_vbo_pos = 0;
GLuint escena_vbo_uv = 0;
GLuint escena_vbo_norm = 0;
GLuint escena_vbo_tan = 0;
// Rangos de vértices para diferentes partes
int vertex_count_walls = 0;
int vertex_count_floors = 0;
int vertex_count_ceilings = 0;

/* -- Variables de juego -- */
// Puntero a la clase Maze que representa el mapa del laberinto
Maze* maze = nullptr; 

// Modulo del pathfinder 
PathfindingModule* enemyPathfinder = nullptr;

// Sistema de partículas global
ParticleSystem particleSystem;

// Mapa por defecto
const char* default_map_path = "bin/data/maze_map.txt";
const int default_side_size = 15;

// Variables de estado del jugador
int player_health = 100;
float player_stamina = 100.0f;
int player_keys = 0;

// Constantes de resistencia
const float MAX_STAMINA = 100.0f; // Resistencia máxima
const float STAMINA_DRAIN_RATE = 15.0f; // Resistencia consumida por segundo
const float STAMINA_REGEN_RATE = 8.0f; // Resistencia regenerada por segundo
const float STAMINA_REGEN_COOLDOWN = 2.0f; // Tiempo de enfriamiento antes de regenerar resistencia
float stamina_regen_timer = 0.0f; // Temporizador para controlar el enfriamiento de la regeneración de resistencia

// Variables de cámara
vec3 cam_pos = vec3(0.0f, 3.0f, 0.0f); // Posición inicial de la cámara (observador)
vec3 cam_target = vec3(0.0f, 0.0f, 1.0f); // Dirección hacia donde mira la cámara
vec3 cam_up = vec3(0.0f, 1.0f, 0.0f); // Vector "arriba" de la cámara
int cam_fov = 62; // Campo de visión inicial
int cam_speed = 3; // Velocidad de movimiento de la cámara
int cam_run_speed = 6; // Velocidad en carrera (Shift)
float aspect_ratio = 4.0f / 3.0f; // Aspect ratio inicial (proporción de la ventana)
float cam_radius = 0.5f; // Radio de colisión de la cámara (como esfera)

// Variables de control con ratón
double last_mouse_x = 0.0; // Última posición X del ratón
double last_mouse_y = 0.0; // Última posición Y del ratón
float mouse_sensitivity = 0.1f; // Sensibilidad
float cam_yaw = 0.0f; // Rotación horizontal (grados)
float cam_pitch = 0.0f; // Rotación vertical (grados)

// Variables de control con teclado
bool keys_pressed[7] = {false, false, false, false, false, false, false}; // W, S, D, A, Q, E, Shift
double last_frame_time = 0.0; // Tiempo del último frame

// Variables de ImGui
bool show_stats = true;
bool show_settings = false;

// Variables de control de reinicio (cambio de mapa)
bool show_loading = false; // Indica si se debe mostrar el mensaje de carga
bool pending_reset = false; // Indica si hay un reset pendiente
bool loading_frame_shown = false; // Indica si ya se mostró el mensaje de carga
const char* pending_map_path = nullptr;
int pending_side_size = 0;

// Variables de fin
bool game_over = false;
double game_over_time = 0.0;

// Variables de control de intensidad de los mapas de texturas
float displacement_intensity = 1.0f;  // Intensidad del desplazamiento (0.0 - 2.0)
float ao_intensity = 1.0f;  // Intensidad del ambient occlusion (0.0 - 2.0)

/**
 * Estructura para almacenar datos de antorchas
 */
struct Torch {
	// Posición en el mundo 3D calculada a partir de x_2D, y_2D y direction_2D
	vec3 position; 
	// Rotación en el eje Y en el mundo 3D
	float rot_y;
	// Coordenada "x" 2D en el mapa .txt
	int x_2D;
	// Coordenada "y" 2D en el mapa .txt
	int y_2D;
	// Dirección en 2D (u, d, r, l) en el mapa .txt
	char direction_2D;
	// Posición de la luz de la antorcha en el mundo 3D
	vec3 lightPos;
	// Posición de la llama en el mundo 3D
	vec3 flamePos;
	// Controlador de partículas para las pavesas del fuego
	ParticleEmitter particleEmitter;

	/**
	 * Constructor para inicializar la antorcha
	 */
	Torch(vec3 position, float rot_y, int x_2D, int y_2D, char direction_2D, vec3 lightPos, vec3 flamePos)
		: position(position), rot_y(rot_y), x_2D(x_2D), y_2D(y_2D), direction_2D(direction_2D), lightPos(lightPos), flamePos(flamePos) {

		// Configuramos el emisor de partículas para la antorcha
		particleEmitter.typeToEmit = ParticleType::Fire;
		particleEmitter.spawnRate = 100.0f; // 100 partículas por segundo
	}
};

/**
 * Estructura para almacenar datos de llaves
 */
struct Keys {
	// Coordenada "x" 2D en el mapa .txt
	int x_2D;
	// Coordenada "y" 2D en el mapa .txt
	int y_2D;
	// Posición 3D en el mundo
	vec3 position;
	// Posición 3D de la luz dorada que acompaña a la llave
	vec3 lightPos;
	// Indica si la llave ya ha sido recogida por el jugador
	bool collected;
	// Controlador de partículas para el aura continua
	ParticleEmitter particleEmitter;

	/**
	 * Constructor para inicializar la llave
	 */
	Keys(int x_2D, int y_2D, vec3 position, vec3 lightPos)
		: x_2D(x_2D), y_2D(y_2D), position(position), lightPos(lightPos), collected(false) {
			particleEmitter.typeToEmit = ParticleType::Key;
			particleEmitter.spawnRate = 40.0f;
		}
};

/**
 * Estructura para almacenar datos de enemigos
 */
struct Enemy {
	// Coordenada "x" 2D en el mapa .txt
	int x_2D;
	// Coordenada "y" 2D en el mapa .txt
	int y_2D;
	// Posición 3D en el mundo
	vec3 position;
	// Rotación en el eje Y en el mundo 3D
	float rot_y;
	// Tiempo del último ataque para controlar el cooldown
	float last_attack_time = 0.0f;
	// Tiempo de cooldown entre ataques (en segundos)
	float attack_cooldown = 1.5f;
	// Camino actual hacia el objetivo
	std::vector<vec3> path;
	// Siguiente waypoint a seguir
	size_t pathIndex;

	/**
	 * Constructor para inicializar el enemigo
	 */
	Enemy (int x_2D, int y_2D, vec3 position, float rot_y)
		: x_2D(x_2D), y_2D(y_2D), position(position), rot_y(rot_y), pathIndex(0) { }
};

/**
 * Estructura para almacenar datos de la salida del laberinto
 */
struct Exit {
	// Coordenada "x" 2D en el mapa .txt
	int x_2D;
	// Coordenada "y" 2D en el mapa .txt
	int y_2D;
	// Dirección en 2D (u, d, r, l) en el mapa .txt
	char direction_2D;
	// Posición en el mundo 3D calculada a partir de x_2D, y_2D y direction_2D
	vec3 position; 
	// Rotación en el eje Y en el mundo 3D
	float rot_y;

	/**
	 * Constructor para inicializar la salida
	 */
	Exit(vec3 position, float rot_y, int x_2D, int y_2D, char direction_2D)
		: position(position), rot_y(rot_y), x_2D(x_2D), y_2D(y_2D), direction_2D(direction_2D) { }
};

/**
 * Estructura para almacenar todas las entidades del juego (antorchas, llaves, enemigo, salida)
 */
struct Entities {
	// Lista de antorchas
	std::vector<Torch> torches;
	// Lista de llaves
	std::vector<Keys> keys;
	// Enemigo
	Enemy enemy = {-1, -1, vec3(-1.0f, -1.0f, -1.0f), 0.0f};
	// Salida del laberinto
	Exit exit = {vec3(-1.0f, -1.0f, -1.0f), 0.0f, -1, -1, 'n'};
};

// Estructura para almacenar las entidades del juego (antorchas, llaves, enemigo, salida)
Entities entities;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 330 core\n" #src

// Vertex Shader (in: pos, color; out: col; uniform: MVP)
const char* vertex_prog = GLSL(
	layout(location = 0) in vec3 pos; // Posición del vértice
	layout(location = 1) in vec2 uv; // Coordenadas de textura
	layout(location = 2) in vec3 normal; // Vector normal de la superficie
	layout(location = 3) in vec3 tangent; // Vector tangente para normal mapping

	out vec2 UV;
	out vec3 FragPos;
	out mat3 TBN;

	uniform mat4 MVP; // Matriz de transformación MVP
	uniform mat4 M;

	void main() { 
		gl_Position = MVP * vec4(pos,1); 
		FragPos = vec3(M * vec4(pos,1));
		UV = uv; 
		vec3 T = normalize(vec3(M * vec4(tangent, 0)));
		vec3 N = normalize(vec3(M * vec4(normal, 0)));
		vec3 B = cross(N, T);
		TBN = mat3(T, B, N);
	}
);

// Fragment Shader (in: col; out: outputColor)
const char* fragment_prog = GLSL(
	in vec2 UV; // Color recibido del vertex shader
	in vec3 FragPos;
	in mat3 TBN;

	uniform sampler2D tex;
	uniform sampler2D normalMap;
	uniform sampler2D displacementMap;
	uniform sampler2D aoMap;
	uniform int numLights;
	uniform vec3 lightPositions[16];
	uniform vec3 lightColors[16];
	uniform float lightIntensities[16];
	uniform vec3 camPos;
	uniform float displacement_intensity;
	uniform float ao_intensity;

	out vec3 outputColor; // Color final que se pintará en la pantalla
	void main() {
		vec3 texColor = texture(tex, UV).rgb;

		vec3 N = texture(normalMap, UV).rgb;
		N = N * 2.0 - 1.0;
		N.xy *= 2.5;
		N = normalize(TBN * N);

		// Cargamos displacement para modular la intensidad del normal y aplicamos intensidad
		float displacement = texture(displacementMap, UV).r * displacement_intensity;
		displacement = clamp(displacement, 0.0, 1.0);
		N = normalize(mix(N, normalize(vec3(0.0, 1.0, 0.0)), displacement * 0.3));

		// Cargamos ambient occlusion y aplicamos intensidad
		float ao = texture(aoMap, UV).r;
		ao = mix(1.0, ao, ao_intensity * 0.5);

		// El ambient ahora incluye el AO
		vec3 ambient = texColor * (0.025 * ao);

		vec3 V = normalize(camPos - FragPos);

		// Normal geométrico que permite saber si la luz está de frente o detrás de la pared
		vec3 geomNormal = normalize(TBN[2]);

		vec3 result = ambient;

		float specularPower = mix(32.0, 2.0, 1.0f);

		float light_range = 6.0; // Rango máximo de la luz
		float light_soft = 2.0; // Rango de suavizado al final del rango máximo

		// Acumular contribución de cada antorcha
		for(int i = 0; i < numLights; i++){
			vec3 L = lightPositions[i] - FragPos;
			float light_dist = length(L);  // Distancia entre la luz y el fragmento
			if (light_dist > light_range + light_soft) continue; // Si el fragmento está fuera del rango, saltamos esta luz

			L = normalize(L);

			// Si la pared le da la espalda a la luz, la saltamos
			if (dot(geomNormal, L) <= 0.0) continue;

			// Difusa
			float diffuse = max(dot(N,L), 0.0);

			// Especular (si no hay luz difusa, no hay especular)
			vec3 R = reflect(-L, N);
			float specular = diffuse <= 0.0f ? 0.0f : pow(max(dot(V, R), 0.0), specularPower) * 0.045;

			// Calculos relativos a la luz del personaje
			float light_cutoff = 1.0 - smoothstep(light_range - light_soft, light_range, light_dist); // Factor de atenuación basado en la distancia
			float light_attenuation = light_cutoff / (1.0 + 0.2 * light_dist + 0.5 * light_dist * light_dist); // Atenuación

			vec3 contribution = (texColor * diffuse * light_attenuation * ao) + vec3(specular * light_attenuation);

			// Aplicamos color e intensidad de la luz (incluye el parpadeo)
			result += contribution * lightColors[i] * lightIntensities[i];
		}

		outputColor = result;
	}
);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CARGA DE DATOS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función para cargar el mapa del laberinto desde un archivo .txt
 * El archivo debe contener valores separados por comas (1 = muro, 0 = vacío)
 * Las filas deben estar en líneas separadas
 * @param filename ruta del archivo .txt con el mapa del laberinto
 * @param side_size tamaño lateral del mapa (cuadrado)
 * @return puntero a un array dinámico con los datos del mapa o nullptr si hubo error al cargar el archivo
 */
int* load_maze_from_file(const char* filename, int side_size) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		printf("CARGA: Error - No se pudo abrir el archivo: %s\n", filename);
		return nullptr;
	}

	int map_size = side_size * side_size;

	// Creamos un array dinámico para almacenar el mapa
	int* map = new int[map_size];

	int index = 0;
	std::string line;

	// Iteramos por línea por línea
	while (std::getline(file, line) && index < map_size) {
		std::istringstream iss(line);
		int value = 0;
		char comma;
		while (index < map_size && value != -1) {
			// Intentamos leer un valor entero
			if (iss >> value) {
				// Insertamos el valor en el array del mapa
				map[index++] = value;
				// Intentamos leer la coma
				if (!(iss >> comma) || comma != ',') {
					break; // Fin de línea o no hay coma
				}
			} else {
				break; // No hay más valores en esta línea
			}
		}
	}

	// Cerramos archivo y verificamos si se leyeron todos los valores esperados
	file.close();

	if (index != map_size) {
		printf("CARGA: Advertencia - El número de valores leídos (%d) no coincide con el tamaño esperado del mapa (%d)\n", index, map_size);
	}

	return map;
}

/**
 * Función para cargar datos de antorchas desde un archivo .txt
 * El formato esperado es: a, int, int, letra (donde letra es u, d, r, l)
 * @param filename ruta del archivo .txt con los datos de las entidades
 * @param maze_rows número de filas del mapa del laberinto
 * @param tile_size tamaño de cada celda del laberinto para calcular posiciones en el mundo 3D
 * @param maze_center_xz coordenada central del laberinto en el eje XZ para calcular posiciones en el mundo 3D
 * @return Entities estructura con los datos de las entidades cargadas (antorchas, llaves, enemigo, salida)
 */
Entities load_entities_from_file(const char* filename, int maze_rows, float tile_size, float maze_center_xz) {
	Entities ent;
	std::ifstream file(filename);
	
	if (!file.is_open()) {
		printf("CARGA: Error - No se pudo abrir el archivo: %s\n", filename);
		return ent;
	}

	// Variables auxiliares
	vec3 position;
	vec3 lightPos;
	vec3 flamePos;
	float rot_y;
	bool enemy_loaded = false;
	bool exit_loaded = false;

	int index = -1;
	std::string line;
	// Iteramos por el archivo línea por línea
	while (std::getline(file, line)) {
		index++;

		// Saltamos las lineas del mapa del laberinto
		if (index < maze_rows){ 
			continue;
		}

		// Eliminamos espacios en blanco al inicio y final
		size_t start = line.find_first_not_of(" \t\r\n");
		
		// Saltamos líneas vacías
		if (start == std::string::npos) continue;  
		
		// Parseamos: a, int, int, char
		std::istringstream iss(line);
		char type, comma;
		int x_2D, y_2D; // Coordenadas 2D en el mapa .txt
		char direction_2D; // Dirección 2D en el mapa .txt (u, d, r, l)

		// Leemos según el formato esperado (tipo, x, y, dir)
		if (iss >> type && iss >> comma && comma == ',' // tipo,
			&& iss >> y_2D && iss >> comma && comma == ',' // y, (fila)
			&& iss >> x_2D) { // x, (columna)
			// Diferenciamos según el tipo de entidad
			switch (type){
				case 'a':
					// Antorcha
					if (iss >> comma && comma == ',' && iss >> direction_2D) {
						// Validamos la dirección
						if (direction_2D == 'u' || direction_2D == 'd' || direction_2D == 'r' || direction_2D == 'l') {
							// Calculamos la posicion 3D, rotación Y, posición de luz y posición de llama
							position = torch_module::compute_world_pos(x_2D, y_2D, direction_2D, tile_size, maze_center_xz);
							rot_y = torch_module::compute_rotation(direction_2D);
							lightPos = lighting::compute_torch_light_pos(x_2D, y_2D, direction_2D, tile_size, maze_center_xz);
							flamePos = flame::compute_position(position, rot_y);

							// Creamos la antorcha y la agregamos a la lista
							ent.torches.emplace_back(
								position,
								rot_y,
								x_2D,
								y_2D,
								direction_2D,
								lightPos,
								flamePos);
							printf("CARGA: Antorcha: x=%d, y=%d, dir=%c\n", x_2D, y_2D, direction_2D);
						} else {
							printf("CARGA: Advertencia - Direccion invalida '%c' en linea: %s\n", direction_2D, line.c_str());
						}
					}
					break;
				case 'k':
					// Llave: calculamos la posición 3D y la posición de su luz
					position = key_module::compute_world_pos(x_2D, y_2D, tile_size, maze_center_xz);
					lightPos = key_module::compute_light_pos(position);

					// Creamos la llave y la agregamos a la lista
					ent.keys.emplace_back(x_2D, y_2D, position, lightPos);
					printf("CARGA: Llave: x=%d, y=%d\n", x_2D, y_2D);
					break;
				case 'e':
					// Enemigo
					if (enemy_loaded) {
						printf("CARGA: Advertencia - Multiple definicion de enemigo en linea (ignorada): %s\n", line.c_str());
						break;
					}
					enemy_loaded = true;

					// Calculamos la posición 3D
					position = enemy::compute_world_pos(x_2D, y_2D, tile_size, maze_center_xz);
					
					// Establecemos los datos del enemigo
					ent.enemy.x_2D = x_2D;
					ent.enemy.y_2D = y_2D;
					ent.enemy.position = position;
					ent.enemy.rot_y = 0.0f;
					printf("CARGA: Enemigo: x=%d, y=%d\n", x_2D, y_2D);
					break;
				case 'x':
					if (exit_loaded) {
						printf("CARGA: Advertencia - Multiple definicion de salida en linea (ignorada): %s\n", line.c_str());
						break;
					}
					exit_loaded = true;

					// Salida
					if (iss >> comma && comma == ',' && iss >> direction_2D) {
						// Validamos la dirección
						if (direction_2D == 'u' || direction_2D == 'd' || direction_2D == 'r' || direction_2D == 'l') {
							// Calculamos la posición 3D y la rotación Y
							position = door::compute_world_pos(x_2D, y_2D, direction_2D, tile_size, maze_center_xz);
							rot_y = door::compute_rotation(direction_2D);

							// Establecemos los datos de la salida
							ent.exit.position = position;
							ent.exit.rot_y = rot_y;
							ent.exit.x_2D = x_2D;
							ent.exit.y_2D = y_2D;
							ent.exit.direction_2D = direction_2D;
							printf("CARGA: Salida: x=%d, y=%d\n", x_2D, y_2D);
						} else {
							printf("CARGA: Advertencia - Direccion invalida '%c' en linea: %s\n", direction_2D, line.c_str());
						}
					}
					break;
				default:
					printf("CARGA: Advertencia - Tipo desconocido '%c' en linea: %s\n", type, line.c_str());
					break;
			}
		}
	}
	
	if (ent.keys.size() != 3 || !enemy_loaded || !exit_loaded) {
		printf("ERROR - CARGA: Se esperaban 3 llaves, 1 enemigo y 1 salida. Encontrados: %zu llaves, %d enemigo, %d salida\n", ent.keys.size(), enemy_loaded, exit_loaded);
		
		// Cerramos el juego y liberamos recursos
		glfwTerminate();
		exit(EXIT_FAILURE);
		return ent;
	}

	file.close();
	printf("CARGA: Antorchas: %zu - Llaves: %zu - Enemigo: %d - Salida: %d\n", ent.torches.size(), ent.keys.size(), enemy_loaded, exit_loaded);
	return ent;
}

/**
 * Función para crear el escenario del laberinto generando cubos para cada pared del mapa 2D.
 * @param map_path ruta del archivo .txt con el mapa del laberinto y datos de las entidades
 * @param side_size tamaño lateral del mapa (cuadrado)
 * @return objeto con VAO y número de vértices para renderizar el escenario del laberinto
 */
objeto crear_escena(const char* map_path, int side_size){
	objeto obj = {};
	GLuint VAO;
	
	// Creamos el laberinto con filas/columnas y tamaño de celda 4.0 unidades
	maze = new Maze(side_size, 4.0f);

	// Cargamos el mapa desde el archivo .txt (1 = muro, 0 = vacío)
	int* map = load_maze_from_file(map_path, side_size);
	if (map == nullptr) {
		printf("ERROR - CARGA: No se pudo cargar el mapa del laberinto: %s\n", map_path);

		// Cerramos el juego y liberamos recursos
		glfwTerminate();
		exit(EXIT_FAILURE);
		return obj;
	}
	
	// Creamos el módulo de pathfinding del enemigo
	enemyPathfinder = new PathfindingModule(side_size, 4.0f);
	enemyPathfinder->setMazeData(map);

	// Cargamos el mapa en la clase Maze
	maze->setMap(map, side_size);

	// Tamaño de cada celda del mapa
	float tile_size = maze->getTileSize(); 
	// Centro del laberinto en coordenadas XZ
	float maze_center_xz = (maze->getColumns() * tile_size) / 2.0f; // Simétrico en XZ

	// Cargamos todas las entidades (antorchas, llaves, enemigo, salida) desde el archivo .txt
	entities = load_entities_from_file(map_path, side_size, tile_size, maze_center_xz);

	// Contamos cuantos cubos necesitamos
	int wall_count = 0; // Paredes
	int empty_count = 0; // Techos/suelos
	for (int i = 0; i < maze->getRows(); i++){
		for(int j = 0; j < maze->getColumns(); j++){
			// Si el valor es 1, es un muro y necesitamos un cubo
			if (maze->getGrid(i, j) == 1){
				wall_count++;
			}
			// Si el valor es 0, es vacío y necesitamos una placa de suelo/techo
			else if (maze->getGrid(i, j) == 0){
				empty_count++;
			}
		}
	}

	// Cada cubo tiene 36 vértices (6 caras x 2 triángulos x 3 vértices)
	// Calculamos el número total de vértices necesarios para todas las paredes + placas de suelo/techo unitarias 
	int total_vertices = (wall_count + (empty_count * 2)) * 36;

	// Creamos arrays para posiciones y colores de los vértices
	GLfloat* pos_data = new GLfloat[total_vertices * 3];
	GLfloat* uv_data = new GLfloat[total_vertices * 2];
	GLfloat* normal_data = new GLfloat[total_vertices * 3];
	GLfloat* tangent_data = new GLfloat[total_vertices * 3];

	// Definimos un cubo unitario centrado en el origen (0,0,0) con tamaño 1x1x1
	GLfloat cube_vertices[36][3] = {
		// Cara frontal (z = 1) - Rojo
		-0.5f, -0.5f,  0.5f,   // tri 1: inf-izq
		 0.5f, -0.5f,  0.5f,   //        inf-der
		 0.5f,  1.0f,  0.5f,   //        sup-der
		-0.5f, -0.5f,  0.5f,   // tri 2: inf-izq
		 0.5f,  1.0f,  0.5f,   //        sup-der
		-0.5f,  1.0f,  0.5f,   //        sup-izq

		// Cara trasera (z = -0.5)  - Verde
		 0.5f, -0.5f, -0.5f,   // tri 1: inf-der
		-0.5f, -0.5f, -0.5f,   //        inf-izq
		-0.5f,  1.0f, -0.5f,   //        sup-izq
		 0.5f, -0.5f, -0.5f,   // tri 2: inf-der
		-0.5f,  1.0f, -0.5f,   //        sup-izq
		 0.5f,  1.0f, -0.5f,   //        sup-der

		// Cara derecha (x = 0.5) - Azul
		 0.5f, -0.5f,  0.5f,   // tri 1: inf-der
		 0.5f, -0.5f, -0.5f,   //        inf-izq
		 0.5f,  1.0f, -0.5f,   //        sup-izq
		 0.5f, -0.5f,  0.5f,   // tri 2: inf-der
		 0.5f,  1.0f, -0.5f,   //        sup-izq
		 0.5f,  1.0f,  0.5f,   //        sup-der

		// Cara izquierda (x = -0.5) - Amarillo
		-0.5f, -0.5f, -0.5f,   // tri 1: inf-der
		-0.5f, -0.5f,  0.5f,   //        inf-izq
		-0.5f,  1.0f,  0.5f,   //        sup-izq
		-0.5f, -0.5f, -0.5f,   // tri 2: inf-der
		-0.5f,  1.0f,  0.5f,   //        sup-izq
		-0.5f,  1.0f, -0.5f,   //        sup-der

		// Cara superior (y = 0.5) - Cian
		-0.5f,  1.0f,  0.5f,   // tri 1: izq-atras
		 0.5f,  1.0f,  0.5f,   //        der-atras
		 0.5f,  1.0f, -0.5f,   //        der-frente
		-0.5f,  1.0f,  0.5f,   // tri 2: izq-atras
		 0.5f,  1.0f, -0.5f,   //        der-frente
		-0.5f,  1.0f, -0.5f,   //        izq-frente

		// Cara inferior (y = -0.5) - Magenta
		-0.5f, -0.5f, -0.5f,   // tri 1: izq-frente
		 0.5f, -0.5f, -0.5f,   //        der-frente
		 0.5f, -0.5f,  0.5f,   //        der-atras
		-0.5f, -0.5f, -0.5f,   // tri 2: izq-frente
		 0.5f, -0.5f,  0.5f,   //        der-atras
		-0.5f, -0.5f,  0.5f    //        izq-atras
	};

	// Colores para cada cara
	GLfloat face_uv[6][2] = {
		{0.0f, 0.0f}, 
		{1.0f, 0.0f}, 
		{1.0f, 1.0f},  
		{0.0f, 0.0f},  
		{1.0f, 1.0f},  
		{0.0f, 1.0f}   
	};

	GLfloat cube_normals[6][3] = {
		{0.0f, 0.0f, 1.0f},
		{0.0f, 0.0f, -1.0f},
		{1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f}
	};

	GLfloat cube_tangents[6][3] = {
		{1.0f, 0.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, 1.0f},
		{1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f}
	};

	// Recorremos el mapa y creamos un cubo por cada pared
	int start_index = 0;
	int vertex_index = 0;
	int ui = 0;
	int ni = 0;
	int ti = 0;
	for (int i = 0; i < maze->getRows(); i++){
		for(int j = 0; j < maze->getColumns(); j++){
			// Si hay muro en esta celda, creamos un cubo
			if (maze->getGrid(i, j) == 1){
				// Posición central del cubo basada en coordenadas del mapa
				float posX = j * tile_size - maze_center_xz;
				float posZ = i * tile_size - maze_center_xz;
				float posY = tile_size / 2.0f; // Altura del cubo

				// Copiamos los vértices del cubo unitario, escalándolos y trasladándolos a su posición
				for (int k = 0; k < 36; k++){
					int v_index = vertex_index; // Índice para el vértice actual
					pos_data[v_index] = cube_vertices[k][0] * tile_size + posX;
					pos_data[v_index + 1] = cube_vertices[k][1] * tile_size + posY;
					pos_data[v_index + 2] = cube_vertices[k][2] * tile_size + posZ;
					vertex_index += 3;

					// Asignamos color basado en la cara
					int face_vert = k % 6; // cada cara 6 vértices
					uv_data[ui] = face_uv[face_vert][0];
					uv_data[ui + 1] = face_uv[face_vert][1];
					ui += 2;

					int face = k / 6;
					normal_data[ni] = cube_normals[face][0];
					normal_data[ni + 1] = cube_normals[face][1];
					normal_data[ni + 2] = cube_normals[face][2];
					ni += 3;

					tangent_data[ti] = cube_tangents[face][0];
					tangent_data[ti + 1] = cube_tangents[face][1];
					tangent_data[ti + 2] = cube_tangents[face][2];
					ti += 3;
				}
			}
		}
	}

	// Guardamos el contador de vértices de paredes
	vertex_count_walls = vertex_index / 3;
	start_index = vertex_index; // Guardamos donde terminan las paredes y empiezan los suelos

	// Crear placas de suelo/techo unitarias para cada celda vacía
	float floor_thickness = 0.5f; // Grosor del suelo/techo
	float scale_y = floor_thickness / 2.0f;

	float floorY = -scale_y; // Altura del suelo
	float roofY = tile_size * 1.5f; // Altura del techo
	
	// Creamos los suelos para cada celda vacía
	for (int i = 0; i < maze->getRows(); i++){
		for(int j = 0; j < maze->getColumns(); j++){
			if (maze->getGrid(i, j) == 0){
				// Posición central de la placa basada en coordenadas del mapa
				float posX = j * tile_size - maze_center_xz;
				float posZ = i * tile_size - maze_center_xz;

				// Copiamos los vértices del cubo unitario
				for (int k = 0; k < 36; k++){
					int v_index = vertex_index;
					pos_data[v_index] = cube_vertices[k][0] * tile_size + posX;
					pos_data[v_index + 1] = cube_vertices[k][1] * scale_y + floorY;
					pos_data[v_index + 2] = cube_vertices[k][2] * tile_size + posZ;
					vertex_index += 3;

					int face_vert = k % 6;
					uv_data[ui] = face_uv[face_vert][0];
					uv_data[ui + 1] = face_uv[face_vert][1];
					ui += 2;

					int face = k / 6;
					normal_data[ni] = cube_normals[face][0];
					normal_data[ni + 1] = cube_normals[face][1];
					normal_data[ni + 2] = cube_normals[face][2];
					ni += 3;

					tangent_data[ti] = cube_tangents[face][0];
					tangent_data[ti + 1] = cube_tangents[face][1];
					tangent_data[ti + 2] = cube_tangents[face][2];
					ti += 3;
				}
			}
		}
	}

	// Guardar el punto donde terminan los suelos y comienzan los techos
	vertex_count_floors = (vertex_index - start_index) / 3;
	start_index = vertex_index; // Guardamos donde terminan los suelos y empiezan los techos

	// Creamos los techos para cada celda vacía
	for (int i = 0; i < maze->getRows(); i++){
		for(int j = 0; j < maze->getColumns(); j++){
			if (maze->getGrid(i, j) == 0){
				// Posición central de la placa basada en coordenadas del mapa
				float posX = j * tile_size - maze_center_xz;
				float posZ = i * tile_size - maze_center_xz;

				// Copiamos los vértices del cubo unitario
				for (int k = 0; k < 36; k++){
					int v_index = vertex_index;
					pos_data[v_index] = cube_vertices[k][0] * tile_size + posX;
					pos_data[v_index + 1] = cube_vertices[k][1] * scale_y + roofY;
					pos_data[v_index + 2] = cube_vertices[k][2] * tile_size + posZ;
					vertex_index += 3;

					int face_vert = k % 6;
					uv_data[ui] = face_uv[face_vert][0];
					uv_data[ui + 1] = face_uv[face_vert][1];
					ui += 2;

					int face = k / 6;
					normal_data[ni] = cube_normals[face][0];
					normal_data[ni + 1] = cube_normals[face][1];
					normal_data[ni + 2] = cube_normals[face][2];
					ni += 3;

					tangent_data[ti] = cube_tangents[face][0];
					tangent_data[ti + 1] = cube_tangents[face][1];
					tangent_data[ti + 2] = cube_tangents[face][2];
					ti += 3;
				}
			}
		}
	}

	// Guardamos el contador de techos
	vertex_count_ceilings = (vertex_index - start_index) / 3;

	// Mandamos posiciones en un VBO
	glGenBuffers(1, &escena_vbo_pos); 
	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_pos);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 3 * sizeof(GLfloat), pos_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Mandamos colores en otro VBO
	glGenBuffers(1, &escena_vbo_uv); 
	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_uv);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 2 * sizeof(GLfloat), uv_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &escena_vbo_norm); 
	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_norm);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 3 * sizeof(GLfloat), normal_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &escena_vbo_tan); 
	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_tan);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 3 * sizeof(GLfloat), tangent_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Creamos y enlazamos el VAO
	glGenVertexArrays(1, &VAO);	
	glBindVertexArray(VAO);

	// Indicamos donde hallar datos de posiciones
	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_pos);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Indicamos donde hallar datos de colores
	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_uv);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_norm);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, escena_vbo_tan);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Cerramos VAO con todo listo para ser pintado
	glBindVertexArray(0);

	// Borramos datos, ya están en la GPU
	delete[] pos_data;
	delete[] uv_data;
	delete[] normal_data;
	delete[] tangent_data;
	delete[] map;

	// Devolvemos objeto VAO + número de vértices en estructura obj
	obj.VAO = VAO; 
	obj.Nv = total_vertices;
	return obj;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     FUNCIONES DE INICIALIZACIÓN
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función para inicializar recursos de renderizado
 */
void init_render_resources() {
	glEnable(GL_DEPTH_TEST);

	// Compilar Shaders
	GLuint VertexShaderID = compilar_shader(vertex_prog, GL_VERTEX_SHADER);
	GLuint FragmentShaderID = compilar_shader(fragment_prog, GL_FRAGMENT_SHADER);

	// Enlazar sharders en el programa final
	prog = glCreateProgram();
	glAttachShader(prog, VertexShaderID);  
	glAttachShader(prog, FragmentShaderID);
	glLinkProgram(prog); 
	check_errores_programa(prog);

	// Limpieza final de los shaders una vez compilado el programa
	glDetachShader(prog, VertexShaderID);  
	glDeleteShader(VertexShaderID);
	glDetachShader(prog, FragmentShaderID);  
	glDeleteShader(FragmentShaderID);

	// Alternativamente usar la funci�n Compile_Link_Shaders().
	//	prog = Compile_Link_Shaders(vertex_prog, fragment_prog); 

	// Indicamos que programa vamos a usar 
	glUseProgram(prog);

	// Cargar texturas de paredes
	tex_brick = cargar_textura("bin/data/brick.jpg", GL_TEXTURE0);
	transfer_int("tex", 0);

	tex_normal = cargar_textura("bin/data/brick_n.jpg", GL_TEXTURE1);
	transfer_int("normalMap", 1);

	tex_displacement = cargar_textura("bin/data/brick_d.jpg", GL_TEXTURE2);
	transfer_int("displacementMap", 2);

	tex_ao = cargar_textura("bin/data/brick_ao.jpg", GL_TEXTURE3);
	transfer_int("aoMap", 3);

	// Cargar texturas de suelo
	tex_floor = cargar_textura("bin/data/sand.jpg", GL_TEXTURE4);
	tex_floor_normal = cargar_textura("bin/data/sand_n.jpg", GL_TEXTURE5);
	tex_floor_displacement = cargar_textura("bin/data/sand_d.jpg", GL_TEXTURE6);
	tex_floor_ao = cargar_textura("bin/data/sand_ao.jpg", GL_TEXTURE7);

	// Cargar texturas de techo 
	tex_ceiling = cargar_textura("bin/data/concrete.jpg", GL_TEXTURE8);
	tex_ceiling_normal = cargar_textura("bin/data/concrete_n.jpg", GL_TEXTURE9);
	tex_ceiling_displacement = cargar_textura("bin/data/concrete_d.jpg", GL_TEXTURE10);
	tex_ceiling_ao = cargar_textura("bin/data/concrete_ao.jpg", GL_TEXTURE11);

	// Inicializamos módulos
	torch_module::init(); // Antorchas
	door::init(); // Puerta de salida
	enemy::init(); // Enemigo
	flame::init(); // Llamas de las antorchas
	key_module::init(); // Llaves
	particleSystem.init(); // Sistema de partículas

	// Volver al programa principal
	glUseProgram(prog);
}

/**
 * Función para inicializar la escena
 * Prepara los datos de los objetos a dibujar, los envía a la GPU
 * Compila los programas a ejecutar en la tarjeta gráfica: vertex shader, fragment shader
 * Configura opciones generales de render de OpenGL
 */
void init_scene() {
	int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height); 
    
	escena_cubica = crear_escena(default_map_path, default_side_size);  // Crear el escenario del laberinto con todas las paredes

	init_render_resources();
}

/**
 * Función para inicializar ImGui
 */
void init_imgui() {
	// Configuración básica de ImGui
	IMGUI_CHECKVERSION();

	// Creamos el contexto
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// Estilo de color oscuro
	ImGui::StyleColorsDark();

	// Configuramos ImGui para que use GLFW y OpenGL3
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     FUNCIONES DE DESTRUCCIÓN
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función para liberar buffers de GPU del escenario actual
 */
void destroy_scene_gpu() {
	if (escena_cubica.VAO != 0) {
		glDeleteVertexArrays(1, &escena_cubica.VAO);
		escena_cubica.VAO = 0;
	}

	escena_cubica.Nv = 0;

	if (escena_vbo_pos != 0) {
		glDeleteBuffers(1, &escena_vbo_pos);
		escena_vbo_pos = 0;
	}

	if (escena_vbo_uv != 0) {
		glDeleteBuffers(1, &escena_vbo_uv);
		escena_vbo_uv = 0;
	}

	if (escena_vbo_norm != 0) {
		glDeleteBuffers(1, &escena_vbo_norm);
		escena_vbo_norm = 0;
	}

	if (escena_vbo_tan != 0) {
		glDeleteBuffers(1, &escena_vbo_tan);
		escena_vbo_tan = 0;
	}
}

/**
 * Función para liberar recursos de renderizado (programas y texturas)
 */
void destroy_render_resources() {
	particleSystem.shutdown();
	torch_module::shutdown();
	flame::shutdown();
	door::shutdown();
	enemy::shutdown();

	if (tex_brick != 0) {
		glDeleteTextures(1, &tex_brick);
		tex_brick = 0;
	}
	if (tex_normal != 0) {
		glDeleteTextures(1, &tex_normal);
		tex_normal = 0;
	}
	if (tex_displacement != 0) {
		glDeleteTextures(1, &tex_displacement);
		tex_displacement = 0;
	}
	if (tex_ao != 0) {
		glDeleteTextures(1, &tex_ao);
		tex_ao = 0;
	}

	if (tex_floor != 0) {
		glDeleteTextures(1, &tex_floor);
		tex_floor = 0;
	}
	if (tex_floor_normal != 0) {
		glDeleteTextures(1, &tex_floor_normal);
		tex_floor_normal = 0;
	}
	if (tex_floor_displacement != 0) {
		glDeleteTextures(1, &tex_floor_displacement);
		tex_floor_displacement = 0;
	}
	if (tex_floor_ao != 0) {
		glDeleteTextures(1, &tex_floor_ao);
		tex_floor_ao = 0;
	}

	if (tex_ceiling != 0) {
		glDeleteTextures(1, &tex_ceiling);
		tex_ceiling = 0;
	}
	if (tex_ceiling_normal != 0) {
		glDeleteTextures(1, &tex_ceiling_normal);
		tex_ceiling_normal = 0;
	}
	if (tex_ceiling_displacement != 0) {
		glDeleteTextures(1, &tex_ceiling_displacement);
		tex_ceiling_displacement = 0;
	}
	if (tex_ceiling_ao != 0) {
		glDeleteTextures(1, &tex_ceiling_ao);
		tex_ceiling_ao = 0;
	}

	if (prog != 0) {
		glDeleteProgram(prog);
		prog = 0;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     FUNCIONES DE REINICIO
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función para reiniciar por completo la escena y cargar un nuevo mapa (sin reiniciar la aplicación)
 */
void reset_scene(const char* map_path, int side_size) {
	destroy_scene_gpu();
	destroy_render_resources();

	if (maze != nullptr) {
		delete maze;
		maze = nullptr;
	}

	entities = Entities();
	
	for (int i = 0; i < 7; i++) {
		keys_pressed[i] = false;
	}

	player_health = 100;
	player_stamina = MAX_STAMINA;
	player_keys = 0;
	game_over = false;
	game_over_time = 0.0;

	cam_pos = vec3(0.0f, 3.0f, 0.0f);
	cam_target = vec3(0.0f, 0.0f, 1.0f);
	cam_up = vec3(0.0f, 1.0f, 0.0f);
	cam_yaw = 0.0f;
	cam_pitch = 0.0f;
	last_frame_time = glfwGetTime();

	escena_cubica = crear_escena(map_path, side_size);
	init_render_resources();
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     FUNCIONES AUXILIARES 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función para verificar si la cámara puede moverse a una nueva posición sin colisionar con las paredes del laberinto
 * @param new_pos Nueva posición a la que se quiere mover la cámara
 * @return true si la cámara puede moverse a la nueva posición sin colisiones, false
 */
bool can_move(vec3& new_pos) {
	// Comprobamos si la nueva posición colisionaría con alguna bounding box
	return !maze->checkCollisionWithBoundingBoxes(new_pos, cam_radius);
}

/**
 * Función para comprobar las condiciones de salida.
 * Si porta las tres llaves pero está lejos, se activa el cambio de rotación de la puerta.
 * Si porta las tres llaves y está cerca, se acaba el juego.
 * @param delta_time Tiempo transcurrido desde el último frame
 * @param current_time Tiempo actual
 */
void check_exit_condition(float delta_time, float current_time) {
	if (player_keys >= 3) {
		if (&entities.exit.position) {
			float dx = cam_pos.x - entities.exit.position.x;
			float dz = cam_pos.z - entities.exit.position.z;
			float dist_sq = dx * dx + dz * dz;

			// Si está cerca de la puerta, activamos fin de juego
			if (dist_sq < 1.0f) {
				game_over = true;
				game_over_time = current_time;
			}
		}	
	}
}

/**
 * Función para comprobar si el jugador está lo bastante cerca para recoger una llave
 * @param k Llave a comprobar
 */
void check_key_pickup(Keys& k) {
    // Radio de recogida　
    const float pickup_radius = 2.0f;
    const float pickup_radius_sq = pickup_radius * pickup_radius;

	float dx = cam_pos.x - k.position.x;
	float dz = cam_pos.z - k.position.z;
	float dist_sq = dx * dx + dz * dz;

	if (dist_sq < pickup_radius_sq) {
		k.collected = true;
		player_keys++;

		// Burst de partículas en la posición visible de la llave
		vec3 burst_pos = key_module::compute_light_pos(k.position);
		const int BURST_COUNT = 300;
		for (int i = 0; i < BURST_COUNT; i++) {
			particleSystem.emit(Particle::CreatePickup(burst_pos));
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     FUNCIONES RENDER Y ACTUALIZACIÓN DE LA ESCENA 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función para enlazar las texturas de paredes
 */
static void bind_wall_textures() {
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex_brick);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, tex_normal);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, tex_displacement);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, tex_ao);
}

/**
 * Función para enlazar las texturas de suelo
 */
static void bind_floor_textures() {
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex_floor);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, tex_floor_normal);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, tex_floor_displacement);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, tex_floor_ao);
}

/**
 * Función para enlazar las texturas de techo
 */
static void bind_ceiling_textures() {
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, tex_ceiling);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, tex_ceiling_normal);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, tex_ceiling_displacement);
	glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, tex_ceiling_ao);
}

/**
 * Función para actualizar la posición de la cámara cada frame.
 * @param delta_time Tiempo transcurrido desde el último frame
 * @param current_time Tiempo actual
 */
void update_controls(float delta_time, float current_time) {
	// Si el juego está pausado, no procesamos controles
	if (game_over || show_settings) return;

	// Detectamos si el jugador se está moviendo (WASD)
	bool is_moving = keys_pressed[0] || keys_pressed[1] || keys_pressed[2] || keys_pressed[3];

	// Solo puede correr si: pulsa Shift + se está moviendo + tiene resistencia 
	bool shift_held = keys_pressed[6];
	bool can_run = shift_held && is_moving && player_stamina > 0.0f;

	// Establecemos la velocidad según si está corriendo o no
	float speed = can_run ? cam_run_speed : cam_speed;

	// Actualizamos la resistencia
	if (can_run) {
		player_stamina -= STAMINA_DRAIN_RATE * (float)delta_time;
		if (player_stamina < 0.0f) player_stamina = 0.0f;
		stamina_regen_timer = current_time; // Reinicia el cooldown al consumir resistencia
	} else {
		// Aplicamos el cooldown de regeneracion
		if (current_time - stamina_regen_timer > STAMINA_REGEN_COOLDOWN) {
			player_stamina += STAMINA_REGEN_RATE * (float)delta_time;
			if (player_stamina > MAX_STAMINA) player_stamina = MAX_STAMINA;
		}
	}

	// Calculamos la distancia a mover en este frame basada en el delta y la velocidad de la cámara
	float movement_distance = speed * (float)delta_time;

	// W: Adelante
	if (keys_pressed[0]) {
		vec3 move = glm::normalize(vec3(cam_target.x, 0.0f, cam_target.z));
		vec3 new_pos = cam_pos + move * movement_distance;
		if (can_move(new_pos)) {
			cam_pos = new_pos;
		}
	}
	// S: Atrás
	if (keys_pressed[1]) {
		vec3 move = glm::normalize(vec3(cam_target.x, 0.0f, cam_target.z));
		vec3 new_pos = cam_pos - move * movement_distance;
		if (can_move(new_pos)) {
			cam_pos = new_pos;
		}
	}
	// D: Derecha
	if (keys_pressed[2]) {
		vec3 right = glm::normalize(glm::cross(cam_target, cam_up));
		vec3 new_pos = cam_pos + right * movement_distance;
		if (can_move(new_pos)) {
			cam_pos = new_pos;
		}
	}
	// A: Izquierda
	if (keys_pressed[3]) {
		vec3 right = glm::normalize(glm::cross(cam_target, cam_up));
		vec3 new_pos = cam_pos - right * movement_distance;
		if (can_move(new_pos)) {
			cam_pos = new_pos;
		}
	}
	// Q: Arriba
	if (keys_pressed[4]) {
		vec3 new_pos = cam_pos + cam_up * movement_distance;
		if (can_move(new_pos)) {
			cam_pos = new_pos;
		}
	}
	// E: Abajo
	if (keys_pressed[5]) {
		vec3 new_pos = cam_pos - cam_up * movement_distance;
		if (can_move(new_pos)) {
			cam_pos = new_pos;
		}
	}
}

/**
 * Función para actualizar el sistema de iluminación cada frame.
 * Actualiza la luz del jugador, de las antorchas, y de las llaves (si no han sido recogidas)
 * @param current_time Tiempo actual, usado para animar la intensidad de las luces
 */
void update_lighting(float current_time) {
	// Construir array de luces
	lighting::clear();

	vec3 torchColor = vec3(1.0f, 0.7f, 0.35f);

	// Luz del jugador
	lighting::add(cam_pos + vec3(0.0f, -0.5f, 0.0f), torchColor, current_time);
	
	// Luz de las antorchas
	for(const Torch& t : entities.torches){
		lighting::add(t.lightPos, torchColor, current_time);
	}
}

/**
 * Función para actualizar las antorchas (y las llamas) cada frame.
 * Dibuja las antorchas, sus llamas y actualiza sus partículas
 * @param delta_time Tiempo transcurrido desde el último frame
 * @param P Matriz de proyección actual
 * @param V Matriz de vista actual
 */
void update_torches(float delta_time, const mat4& P, const mat4& V) {
	// Dibujamos antorchas según entidades cargadas del mapa
	const float torch_scale = 0.6f;
	for(Torch& t : entities.torches){
		torch_module::draw(t.position, torch_scale, t.rot_y, P, V, cam_pos);

		// La llama va por encima del torch + adelante (en la dirección del pasillo)
		flame::draw(t.flamePos, 2.0f, P, V);

		// Actualizamos las partículas de la antorcha
		t.particleEmitter.update(delta_time, t.flamePos, particleSystem);
	}
}

/**
 * Función para actualizar las llaves cada frame.
 * Añade la luz de las llaves no recogidas, dibuja las llaves, actualiza sus partículas y comprueba si el jugador las puede recoger
 * @param delta_time Tiempo transcurrido desde el último frame
 * @param current_time Tiempo actual
 * @param P Matriz de proyección actual
 * @param V Matriz de vista actual
 */
void update_keys(float delta_time, float current_time, const mat4& P, const mat4& V){
	// Luz de las llaves
	float keyT     = (float)glfwGetTime();
	float keyPulse = 1.0f + 0.12f * sin(keyT * 0.9f) + 0.04f * sin(keyT * 2.6f);
	vec3  keyColor = vec3(0.5f, 0.85f, 0.3f) * 1.8f * keyPulse;

	// Luz dorada de las llaves (solo si no han sido recogidas)
	// for(const Keys& k : entities.keys){
	// 	if (!k.collected) {
	// 		lighting::add(k.lightPos, keyColor);
	// 	}
	// }

	// Dibujamos llaves según entidades cargadas del mapa
	for(Keys& k : entities.keys){
		if (!k.collected) {
			key_module::draw(k.position, 0.7f, (float)current_time, P, V, cam_pos);

			// Actualizamos las partículas de la llave
			k.particleEmitter.update(delta_time, k.lightPos, particleSystem);

			// Comprobamos si el jugador está lo bastante cerca para recoger la llave
			check_key_pickup(k);

			lighting::add(k.lightPos, keyColor, current_time);
		}
	}
}

/**
 * Función para dibujar la puerta de salida cada frame.
 * @param P Matriz de proyección actual
 * @param V Matriz de vista actual
 */
void update_exit(const mat4& P, const mat4& V) {
    door::draw(entities.exit.position, 2.0f, entities.exit.rot_y, P, V, cam_pos);
}


/**
 * Función para actualizar el enemigo cada frame.
 * Se calcula el movimiento del enemigo usando A* y se dibuja en su nueva posición.
 * @param delta_time Tiempo transcurrido desde el último frame
 * @param P Matriz de proyección actual
 * @param V Matriz de vista actual
 */
void update_enemy(float delta_time, float current_time, const mat4& P, const mat4& V) {
	// Si el juego está pausado, fijamos la posición del enemigo
	if (game_over || show_settings) {
		enemy::draw(entities.enemy.position, 2.0f, entities.enemy.rot_y, P, V, cam_pos);
		return;
	}

	// Si no hay ruta, o se ha llegado al final de la actual, calculamos una nueva ruta hacia el jugador
	if (entities.enemy.path.empty() || entities.enemy.pathIndex >= entities.enemy.path.size()) {
		entities.enemy.path = enemyPathfinder->findPath(entities.enemy.position.x, entities.enemy.position.z, cam_pos.x, cam_pos.z);
		entities.enemy.pathIndex = 0;
	}

	// Si hay una ruta, intentamos avanzar hacia el siguiente waypoint
	if (!entities.enemy.path.empty() && entities.enemy.pathIndex < entities.enemy.path.size()) {
		const vec3& waypoint = entities.enemy.path[entities.enemy.pathIndex];
		float enemyY = entities.enemy.position.y; // Mantenemos altura fija
		vec3 delta = vec3(waypoint.x - entities.enemy.position.x, 0.0f, waypoint.z - entities.enemy.position.z);
		float distance = glm::length(vec2(delta.x, delta.z));
		float enemySpeed = 1.5f;

		// Diferenciamos la distancia al waypoint
		if (distance < 0.05f) {
			// Si estamos cerca, avanzamos al siguiente waypoint
			entities.enemy.position = waypoint;
			entities.enemy.pathIndex++;
		} else {
			// Si estamos lejos, continuamos avanzando hacia el waypoint actual
			vec3 direction = glm::normalize(delta);
			entities.enemy.position += direction * enemySpeed * (float)delta_time;
		}

		// Mantenemos altura fija, y rotación hacia el jugador
		entities.enemy.position.y = enemyY;
		float to_player_x = cam_pos.x - entities.enemy.position.x;
		float to_player_z = cam_pos.z - entities.enemy.position.z;
		float distance_to_player = (to_player_x * to_player_x + to_player_z * to_player_z);
		if (distance_to_player > 0.005f) {
			entities.enemy.rot_y = atan2(to_player_x, to_player_z);
		}

		// Si el enemigo está lo bastante cerca, reducimos la salud del jugador (con cooldown)
		if (distance_to_player < 4.1f) {
			// Verificamos si ha pasado el tiempo de cooldown
			if (current_time - entities.enemy.last_attack_time > entities.enemy.attack_cooldown) {
				entities.enemy.last_attack_time = current_time;
				player_health -= 10;

				// Si la vida llega a 0, el juego termina
				if (player_health <= 0) {
					player_health = 0;

					game_over = true;
					game_over_time = current_time;
				}
			}
		}
	}

	// Dibujamos el enemigo en su nueva posición
	enemy::draw(entities.enemy.position, 2.0f, entities.enemy.rot_y, P, V, cam_pos);
}

/**
 * Función para actualizar el sistema de partículas cada frame.
 * Incluye cálculo dinamico de la posición NDC del icono de llaves para que las partículas de recogida 
 * apunten siempre hacia el icono, independientemente del tamaño de la ventana.
 * @param delta_time Tiempo transcurrido desde el último frame
 * @param P Matriz de proyección actual
 * @param V Matriz de vista actual
 */
void update_particles(float delta_time, const mat4& P, const mat4& V) {
	// Calculamos la posición NDC del icono de llaves dinámicamente según el tamaño actual de la ventana
	ImGuiIO& io_target = ImGui::GetIO();
	float scr_w = io_target.DisplaySize.x;
	float scr_h = io_target.DisplaySize.y;
	particleSystem.pickup_target_ndc = vec2(
		1.0f - 142.0f / scr_w,
		1.0f - 76.0f / scr_h 
	);

	// Actualizamos sistema de partículas
	particleSystem.update((float)delta_time);

	// Dibujamos partículas
	particleSystem.render(P, V);
}

/**
 * Función para renderizar la interfaz de configuración.
 */
void renderSettingsPanel() {
	ImGuiIO& io = ImGui::GetIO();
	// Posición del panel en centro de la pantalla y tamaño prefijado (siempre que aparezca)
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x / 2.0f - 200, io.DisplaySize.y / 2.0f - 150), ImGuiCond_Appearing);
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);
    
	// Si se muestra el panel, renderizamos su contenido (sin movimiento ni colapso)
    if (show_settings && ImGui::Begin("Configuración", &show_settings, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
		if (ImGui::Button("Mapa 1", ImVec2(90, 0))) {
			show_settings = false;
			show_loading = true;
			loading_frame_shown = false;
			game_over = false;
			game_over_time = 0.0;

			pending_reset = true;
			pending_map_path = "bin/data/maze_map.txt";
			pending_side_size = 15;
        }
		ImGui::SameLine(); // En horizontal al botón de aceptar
		if (ImGui::Button("Mapa 2", ImVec2(90, 0))) {
			show_settings = false;
			show_loading = true;
			loading_frame_shown = false;
			game_over = false;
			game_over_time = 0.0;

			pending_reset = true;
			pending_map_path = "bin/data/maze_map_2.txt";
			pending_side_size = 15;
        }
		ImGui::SameLine(); // En horizontal al botón de aceptar
		if (ImGui::Button("Mapa 3", ImVec2(90, 0))) {
			show_settings = false;
			show_loading = true;
			loading_frame_shown = false;
			game_over = false;
			game_over_time = 0.0;

			pending_reset = true;
			pending_map_path = "bin/data/maze_map_3.txt";
			pending_side_size = 15;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

		// Slider para el FOV (Field of View)
        ImGui::SliderInt("FOV (Campo de Visión)", &cam_fov, 55, 100);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Slider para velocidad de movimiento
        ImGui::SliderInt("Velocidad de Movimiento", &cam_speed, 1, 10);
        
        // Slider para velocidad de carrera
        ImGui::SliderInt("Velocidad de Carrera", &cam_run_speed, 2, 15);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Slider para sensibilidad del ratón
        ImGui::SliderFloat("Sensibilidad del Ratón", &mouse_sensitivity, 0.01f, 0.5f);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Controles para mapas de texturas
        ImGui::Text("Mapas de Texturas");
        ImGui::SliderFloat("Intensidad Displacement", &displacement_intensity, 0.0f, 2.0f);
        ImGui::SliderFloat("Intensidad AO", &ao_intensity, 0.0f, 2.0f);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

		// Botón para aceptar cambios y cerrar panel (verde)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 1.0f, 0.3f, 1.0f)); 
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
        if (ImGui::Button("Aceptar", ImVec2(180, 0))) {
            show_settings = false;
        }
		ImGui::PopStyleColor(3);

		// Botón para restablecer valores por defecto (naranja)
		ImGui::SameLine(); // En horizontal al botón de aceptar
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.4f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.4f, 0.0f, 1.0f));
		if (ImGui::Button("Restablecer", ImVec2(180, 0))) {
			cam_fov = 62;
			cam_speed = 3;
			cam_run_speed = 4;
			mouse_sensitivity = 0.1f;
			displacement_intensity = 1.0f;
			ao_intensity = 1.0f;
		}
		ImGui::PopStyleColor(3);

		// Botón para cerrar el juego (rojo)
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
		if (ImGui::Button("Cerrar Juego", ImVec2(380, 0))) {
			glfwSetWindowShouldClose(window, true);
		}
		ImGui::PopStyleColor(3);
        
        ImGui::End();
    }
}

/**
 * Función para renderizar la interfaz del juego.
 * @param io Referencia a la estructura i/o de ImGui
 */
void renderGameUI(ImGuiIO& io) {
	// Tamaño actual de la ventana 
	const float SCR_W = io.DisplaySize.x;
	const float SCR_H = io.DisplaySize.y;

    const float HUD_W = 340.0f; // Ancho del panel
	const float HUD_H = 40.0f; // Alto del panel
	const float ICON = 22.0f; // Tamaño de icono
	const float GAP_ICON_TXT = 6.0f; // Separación entre un icono y texto
	const float GAP_GROUP = 24.0f; // Separación ente indicadores distintos
	const float STAMINA_BAR_W = 70.0f; // Ancho de la barra de resistencia
	const float STAMINA_BAR_H = 8.0f; // Alto de la barra de resistencia

	// Panel de estadísticas en esquina superior derecha, con tamaño fijo (siempre)
	ImGui::SetNextWindowPos(ImVec2(SCR_W - HUD_W, 10), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(HUD_W, HUD_H), ImGuiCond_Always);

    ImGuiWindowFlags hud_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav;

    if (show_stats && ImGui::Begin("HUD", nullptr, hud_flags)) {
		// Obtenemos el draw list de la ventana actual
		ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();

        const ImU32 COL_GREEN = IM_COL32( 90, 200,90, 255); // Verde para HP
        const ImU32 COL_YELLOW = IM_COL32(230, 200,60, 255); // Amarillo para llave
        const ImU32 COL_BLUE = IM_COL32( 80, 180, 255, 255); // Azul brillante para resistencia
        const ImU32 COL_BLUE_DIM = IM_COL32( 25, 55, 105, 220); // Azul oscuro para fondo de barra
        const ImU32 COL_BLUE_BORDER = IM_COL32( 60, 130, 200, 255); // Azul medio para borde
        const ImU32 COL_WHITE = IM_COL32(255, 255, 255, 255); // Blanco para los textos

        float x = origin.x;
        float y = origin.y + (HUD_H - ICON) * 0.5f;
        char buf[32];

        // Información de HP
        dl->AddRectFilled(ImVec2(x, y + ICON*0.35f),
                          ImVec2(x + ICON, y + ICON*0.65f), COL_GREEN, 3.0f);
        dl->AddRectFilled(ImVec2(x + ICON*0.35f, y),
                          ImVec2(x + ICON*0.65f, y + ICON), COL_GREEN, 3.0f);

        snprintf(buf, sizeof(buf), "%d/100", player_health);
        ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(x + ICON + GAP_ICON_TXT, y + (ICON - ts.y)*0.5f),
                    COL_WHITE, buf);

        x += ICON + GAP_ICON_TXT + ts.x + GAP_GROUP;

        // Información de resistencia
        ImVec2 bolt_upper[4] = {
            ImVec2(x + 0.60f*ICON, y + 0.05f*ICON),
            ImVec2(x + 0.65f*ICON, y + 0.05f*ICON),
            ImVec2(x + 0.50f*ICON, y + 0.62f*ICON), 
            ImVec2(x + 0.15f*ICON, y + 0.62f*ICON) 
        };
        dl->AddConvexPolyFilled(bolt_upper, 4, COL_BLUE);
        ImVec2 bolt_lower[4] = {
            ImVec2(x + 0.40f*ICON, y + 0.38f*ICON), 
            ImVec2(x + 0.75f*ICON, y + 0.38f*ICON), 
            ImVec2(x + 0.30f*ICON, y + 0.95f*ICON),
            ImVec2(x + 0.25f*ICON, y + 0.95f*ICON)
        };
        dl->AddConvexPolyFilled(bolt_lower, 4, COL_BLUE);

        float bar_x = x + ICON + GAP_ICON_TXT;
        float bar_y = y + (ICON - STAMINA_BAR_H) * 0.5f;
        float fill_pct = player_stamina / MAX_STAMINA;
        if (fill_pct < 0.0f) fill_pct = 0.0f;
        if (fill_pct > 1.0f) fill_pct = 1.0f;

        dl->AddRectFilled(ImVec2(bar_x, bar_y),
                          ImVec2(bar_x + STAMINA_BAR_W, bar_y + STAMINA_BAR_H),
                          COL_BLUE_DIM, 2.0f);
        if (fill_pct > 0.0f) {
            dl->AddRectFilled(ImVec2(bar_x, bar_y),
                              ImVec2(bar_x + STAMINA_BAR_W * fill_pct, bar_y + STAMINA_BAR_H),
                              COL_BLUE, 2.0f);
        }
        dl->AddRect(ImVec2(bar_x, bar_y),
                    ImVec2(bar_x + STAMINA_BAR_W, bar_y + STAMINA_BAR_H),
                    COL_BLUE_BORDER, 2.0f, 0, 1.0f);

        snprintf(buf, sizeof(buf), "%d", (int)player_stamina);
        ImVec2 stamina_ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(bar_x + STAMINA_BAR_W + GAP_ICON_TXT, y + (ICON - stamina_ts.y)*0.5f),
                    COL_WHITE, buf);

        x += ICON + GAP_ICON_TXT + STAMINA_BAR_W + GAP_ICON_TXT + stamina_ts.x + GAP_GROUP;

        // Información de Llaves
        ImVec2 ring_c(x + ICON*0.30f, y + ICON*0.5f);
        dl->AddCircle(ring_c, ICON*0.25f, COL_YELLOW, 16, 2.5f);
        dl->AddRectFilled(ImVec2(ring_c.x + ICON*0.20f, y + ICON*0.42f),
                          ImVec2(x + ICON, y + ICON*0.58f), COL_YELLOW);
        dl->AddRectFilled(ImVec2(x + ICON*0.80f, y + ICON*0.58f),
                          ImVec2(x + ICON, y + ICON*0.75f), COL_YELLOW);

        snprintf(buf, sizeof(buf), "%d/3", player_keys);
        dl->AddText(ImVec2(x + ICON + GAP_ICON_TXT, y + (ICON - ts.y)*0.5f),
                    COL_WHITE, buf);

        ImGui::End();
    }

    // Si se muestra el panel de configuración llamamos a su render
	if (show_settings){
        renderSettingsPanel();
    }

    // Mensaje de carga centrado
    if (show_loading) {
        ImGui::SetNextWindowPos(ImVec2(SCR_W / 2.0f, SCR_H / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::Begin("Cargando", nullptr,
            ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoBackground)) {
            ImGui::Text("cargando");
            ImGui::End();
        }
    }

	// Mensaje de fin de partida centrado
	if (game_over && !show_loading) {
		ImGui::SetNextWindowPos(ImVec2(SCR_W / 2.0f, SCR_H / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::Begin("Game Over", nullptr,
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoBackground)) {
			ImGui::Text(player_health <= 0 ? "has muerto" : "has ganado");
			ImGui::End();
		}
	}
}

/**
 * Función para renderizar la escena en cada frame.
 */
void render_scene()
{
	// Evitamos procesar si la ventana está minimizada
	if (ANCHO == 0 || ALTO == 0) {
		return;
	}

	// Si hay reset pendiente y ya se ha mostrado el mensaje de carga, hacemos el reset
	if (pending_reset && loading_frame_shown) {
		reset_scene(pending_map_path, pending_side_size);

		show_loading = false;
		loading_frame_shown = false;

		pending_reset = false;
		pending_map_path = nullptr;
		pending_side_size = 0;
	}
	else if (pending_reset && !loading_frame_shown) {
		// Hay reset pendiente pero NO se ha mostrado mensaje de carga, esperamos a mostrarlo en el siguiente frame
		loading_frame_shown = true;
	}
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Especifica color para el fondo oscuro (RGB+alfa)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpia buffers de color y profundidad

	// Calculamos delta time para movimientos y animaciones suaves (e independiente de FPS)
	double current_time = glfwGetTime();
	double delta_time = current_time - last_frame_time;
	last_frame_time = current_time;
	if (game_over) {
		delta_time = 0.0;
		current_time = game_over_time;
	}

	update_controls(delta_time, (float)current_time);

	check_exit_condition(delta_time, (float)current_time);

	///////// Actualizacion matrices M, V, P  /////////	
	mat4 P, V, M;
	P = perspective(glm::radians((float)cam_fov), aspect_ratio, 0.1f, 50.0f);
	vec3 target = cam_pos + cam_target; // Punto hacia el que mira la cámara
	V = lookAt(cam_pos, target, cam_up); // Pos camara, Lookat relativo a dirección, head up
	M = glm::mat4(1.0f); // Matriz identidad para el cubo (sin transformaciones)
	
	glUseProgram(prog);

	transfer_mat4("MVP", P * V * M);
	transfer_mat4("M", M);
	transfer_vec3("camPos", cam_pos);
	transfer_float("displacement_intensity", displacement_intensity);
	transfer_float("ao_intensity", ao_intensity);
	
	update_lighting((float)current_time);

	update_keys(delta_time, (float)current_time, P, V);

	update_exit(P, V);

	update_enemy(delta_time, current_time, P, V);

	lighting::upload_to_shader(prog); // Incluye glUseProgram(prog);

	glBindVertexArray(escena_cubica.VAO); // Activamos VAO del cubo
	
	// Renderizamos paredes, suelos y techos
	bind_wall_textures();
	glDrawArrays(GL_TRIANGLES, 0, vertex_count_walls);
	bind_floor_textures();
	glDrawArrays(GL_TRIANGLES, vertex_count_walls, vertex_count_floors);
	bind_ceiling_textures();
	glDrawArrays(GL_TRIANGLES, vertex_count_walls + vertex_count_floors, vertex_count_ceilings);
	
	glBindVertexArray(0); // Desconectamos VAO

	update_torches(delta_time, P, V);

	update_particles(delta_time, P, V);

	// Renderizamos las interfaces con ImGui
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	renderGameUI(io);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	// Mostramos/ocultamos el cursor cuando hay menú abierto
	if (show_settings || game_over) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	} else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     PROGRAMA PRINCIPAL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Programa principal
 * @param argc Número de argumentos
 * @param argv Array de argumentos
 * @return Código de salida
 */
int main(int argc, char* argv[])
{
	init_GLFW(); // Inicializa lib GLFW
	window = Init_Window(prac); // Crea ventana usando GLFW, asociada a un contexto OpenGL	X.Y
	load_Opengl(); // Carga funciones de OpenGL, comprueba versión.
	init_scene(); // Prepara escena
	asigna_funciones_callback(window); // Registramos callbacks de entrada
	init_imgui(); // Inicializamos ImGui
	
	// Ocultamos y capturamos el cursor en la ventana para control de cámara con ratón
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	// Hacemos que se sincronice con la tasa de refresco del monitor
	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		render_scene();
		glfwSwapBuffers(window);
		glfwPollEvents();
		show_info();
	}

	// Limpiamos ImGui y GLFW
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

/**
 * Función para mostrar información en el título de la ventana
 */
void show_info()
{
	static int fps = 0;
	static double last_tt = 0;
	double elapsed, tt;
	char nombre_ventana[128];   // buffer para modificar titulo de la ventana

	fps++; tt = glfwGetTime();  // Contador de tiempo en segundos 

	elapsed = (tt - last_tt);
	if (elapsed >= 0.5)  // Refrescar cada 0.5 segundo
	{
		sprintf_s(nombre_ventana, 128, "%s: %4.0f FPS @ %d x %d", prac, fps / elapsed, ANCHO, ALTO);
		glfwSetWindowTitle(window, nombre_ventana);
		last_tt = tt; fps = 0;
	}

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     FUNCIONES CALLBACK
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Función que procesa el cambio de tamaño en la ventana
 * @param window puntero a la ventana GLFW
 * @param width nuevo ancho de la ventana
 * @param height nuevo alto de la ventana
 */
void ResizeCallback(GLFWwindow* window, int width, int height)
{
	glfwGetFramebufferSize(window, &width, &height);
	
	// Evitamos procesar si la ventana está minimizada (tamaño 0)
	if (width == 0 || height == 0) {
		return;
	}
	
	glViewport(0, 0, width, height);
	ALTO = height;	
	ANCHO = width;

	// Actualizamos aspect ratio para la matriz de proyeccion
	aspect_ratio = (float)ANCHO / (float)ALTO; 
}

/**
 * Función que procesa las pulsaciones de teclas para controlar el movimiento de la cámara
 * @param window puntero a la ventana GLFW
 * @param key código de la tecla pulsada
 * @param code código
 * @param action tipo de acción
 * @param mode estado de las teclas modificadoras (Shift, Ctrl, Alt)
 */
static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
	if (game_over) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			show_settings = true;
		}
		return;
	}

	// Si ImGui está mostrando el menú de configuración, no procesamos movimientos
	if (show_settings) {
		// Solo procesamos ESC para cerrar el menú
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			show_settings = false;
		}

		// Y establecemos resto de teclas en false para evitar que queden "pulsadas" al cerar el menú
		for (int i = 0; i < 7; i++) {
			keys_pressed[i] = false;
		}

		return;
	}

	// Actualizamos el estado de las teclas de movimiento
	bool is_pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
	
	switch (key) {
		case GLFW_KEY_ESCAPE:
			// Si se pulsa ESC, mostramos el menú de configuración
			if (action == GLFW_PRESS) {
				show_settings = true;
			}
			break;
		case GLFW_KEY_W:
			keys_pressed[0] = is_pressed; // W: Adelante
			break;
		case GLFW_KEY_S:
			keys_pressed[1] = is_pressed; // S: Atrás
			break;
		case GLFW_KEY_D:
			keys_pressed[2] = is_pressed; // D: Derecha
			break;
		case GLFW_KEY_A:
			keys_pressed[3] = is_pressed; // A: Izquierda
			break;
		case GLFW_KEY_Q:
			keys_pressed[4] = is_pressed; // Q: Arriba
			break;
		case GLFW_KEY_E:
			keys_pressed[5] = is_pressed; // E: Abajo
			break;
		case GLFW_KEY_LEFT_SHIFT:
			keys_pressed[6] = is_pressed; // Shift: Correr
			break;
		default:
			break;
	}
}

/**
 * Función que procesa el movimiento del ratón para controlar la orientación de la cámara
 * @param window puntero a la ventana GLFW
 * @param xpos, ypos posición actual del cursor en la ventana
 */
static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	// Si ImGui está mostrando el menú de configuración, no procesamos movimientos
	if (show_settings || game_over) {
		// Actualizamos la posición del ratón para evitar saltos al cerrar el menú
		last_mouse_x = xpos;
		last_mouse_y = ypos;
		return;
	}

	// Calculamos el delta del movimiento actual respecto a la posición anterior y aplicamos sensibilidad
	double delta_x = (xpos - last_mouse_x) * mouse_sensitivity;
	double delta_y = (ypos - last_mouse_y) * mouse_sensitivity;

	// Actualizamos la última posición
	last_mouse_x = xpos;
	last_mouse_y = ypos;
	
	// Actualizamos yaw y pitch de la cámara
	cam_yaw += delta_x; // Yaw aumenta hacia derecha, disminuye hacia izquierda
	cam_pitch -= delta_y; // Y aumenta hacia abajo en pantalla

	// Limitamos pitch para evitar vueltas completas al llegar al límite superior o inferior
	if (cam_pitch > 89.0f)
		cam_pitch = 89.0f;
	else if (cam_pitch < -89.0f)
		cam_pitch = -89.0f;
	
	// Calculamos el vector de dirección en coordenadas cartesianas 3D
	vec3 direction;
	direction.x = cos(glm::radians(cam_yaw)) * cos(glm::radians(cam_pitch));
	direction.y = sin(glm::radians(cam_pitch));
	direction.z = sin(glm::radians(cam_yaw)) * cos(glm::radians(cam_pitch));
	
	cam_target = glm::normalize(direction);
}

/**
 * Función que asigna las funciones callback para los eventos de ventana, teclado y ratón
 * @param window puntero a la ventana GLFW
 */
void asigna_funciones_callback(GLFWwindow* window)
{
	glfwSetWindowSizeCallback(window, ResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
}
