/**
 * Proyecto final de GpO
 * Título: Dungeon Crawler 3D
 * ATG, 2019
 */

#include <GpO.h>
#include <vector>
#include <maze.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GPO_assimp_aux.h>
#include <flame.h>
#include <lighting.h>
#include <torch_module.h>
#include <ParticleEmitter.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     VARIABLES GLOBALES 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tamaño inicial ventana
int ANCHO = 800, ALTO = 600; 

// Título de la ventana
const char* prac = "OpenGL (GpO)"; 

// Puntero a la clase Maze que representa el mapa del laberinto
Maze* maze; 

// Sistema de partículas global, encargado de gestionar todas las partículas del juego
ParticleSystem particleSystem;

// Variables de estado del jugador
int player_health = 100;
int player_mana = 50;
int player_keys = 0;

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
};

/**
 * Estructura para almacenar datos de enemigos
 */
struct Enemy {
	// Coordenada "x" 2D en el mapa .txt
	int x_2D;
	// Coordenada "y" 2D en el mapa .txt
	int y_2D;
};

/**
 * Estructura para almacenar datos de la salida del laberinto
 */
struct Exit {
	// Coordenada "x" 2D en el mapa .txt
	int x_2D;
	// Coordenada "y" 2D en el mapa .txt
	int y_2D;
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
	Enemy enemy = {-1, -1};
	// Salida del laberinto
	Exit exit = {-1, -1};
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
	uniform vec3 camPos;
	uniform float displacement_intensity;
	uniform float ao_intensity;
	uniform float time;

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
		vec3 ambient = texColor * (0.05 * ao);

		vec3 V = normalize(camPos - FragPos);

		vec3 result = ambient;

		float specularPower = mix(32.0, 2.0, 1.0f);

		float light_range = 6.0; // Rango máximo de la luz
		float light_soft = 2.0; // Rango de suavizado al final del rango máximo

		// Acumular contribución de cada antorcha
		for(int i = 0; i < numLights; i++){
			vec3 L = lightPositions[i] - FragPos;
			float light_dist = length(L);  // Distancia entre la luz y el fragmento
			L = normalize(L);

			//Difusa
			float diffuse = max(dot(N,L), 0.0);

			// Especular 
			vec3 R = reflect(-L, N);
			float specular = pow(max(dot(V, R), 0.0), specularPower) * 0.045;

			// Calculos relativos a la luz del personaje
			float light_cutoff = 1.0 - smoothstep(light_range - light_soft, light_range, light_dist); // Factor de atenuación basado en la distancia
			float light_attenuation = light_cutoff / (1.0 + 0.2 * light_dist + 0.5 * light_dist * light_dist); // Atenuación

			// Cambio pseudoaleatorio en la intensidad de cada luz para simular el parpadeo de las llamas
			float intensity = 1.0;
			if (i > 0) { // Excepto luz del personaje
				float seed = fract(i * 12.9898 + i * 78.233 - i * 45.164); // Semilla basada en índice
				float phase = seed * 6.2831853; // Fase "base"
				float f1 = 1.1 + seed * 0.9; // Frecuencia 1
				float f2 = 2.7 + seed * 1.3; // Frecuencia 2
				float f3 = 4.5 + seed * 2.1; // Frecuencia 3

				// Aplicamos a la intensidad la suma de 3 ondas sinusoidales pseudoaleatorias
				intensity = 0.8
					+ 0.25 * sin(time * f1 + phase)
					+ 0.10 * sin(time * f2 + phase * 1.7)
					+ 0.05 * sin(time * f3 + phase * 2.3);

				// Limitamos intensidad entre 0.5 y 1.2
				intensity = clamp(intensity, 0.5, 1.2); 
			}

			vec3 contribution = (texColor * ((ambient * light_cutoff) + 1.5 * diffuse * light_attenuation * ao) + vec3(specular * light_attenuation)) * lightColors[i];

			result += contribution * intensity;
		}

		outputColor = result;
	}
);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE AND DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLFWwindow* window;
GLuint prog;
objeto escena_cubica; // Objeto para el escenario del laberinto


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
			&& iss >> x_2D && iss >> comma && comma == ',' // x,
			&& iss >> y_2D) { // y
			// Diferenciamos según el tipo de entidad
			switch (type){
				case 'a':
					// Antorcha
					if (iss >> comma && comma == ',' && iss >> direction_2D) {
						// Validamos la dirección
						if (direction_2D == 'u' || direction_2D == 'd' || direction_2D == 'r' || direction_2D == 'l') {
							// Creamos el objeto y lo agregamos a la lista
							vec3 position = torch_module::compute_world_pos(x_2D, y_2D, direction_2D, tile_size, maze_center_xz);
							float rot_y = torch_module::compute_rotation(direction_2D);
							vec3 lightPos = lighting::compute_torch_light_pos(x_2D, y_2D, direction_2D, tile_size, maze_center_xz);
							vec3 flamePos = flame::compute_position(position, rot_y);

							Torch torch = Torch(
								position,
								rot_y,
								x_2D,
								y_2D,
								direction_2D,
								lightPos,
								flamePos
							);

							ent.torches.push_back(torch);
							printf("CARGA: Antorcha: x=%d, y=%d, dir=%c\n", x_2D, y_2D, direction_2D);
						} else {
							printf("CARGA: Advertencia - Direccion invalida '%c' en linea: %s\n", direction_2D, line.c_str());
						}
					}
					break;
				case 'k':
					// Llave
					Keys key;
					key.x_2D = x_2D;
					key.y_2D = y_2D;
					ent.keys.push_back(key);
					printf("CARGA: Llave: x=%d, y=%d\n", x_2D, y_2D);
					break;
				case 'e':
					Enemy enemy;
					enemy.x_2D = x_2D;
					enemy.y_2D = y_2D;
					ent.enemy = enemy;
					printf("CARGA: Enemigo: x=%d, y=%d\n", x_2D, y_2D);
					break;
				case 'x':
					Exit exit;
					exit.x_2D = x_2D;
					exit.y_2D = y_2D;
					ent.exit = exit;
					printf("CARGA: Salida: x=%d, y=%d\n", x_2D, y_2D);
					break;
				default:
					printf("CARGA: Advertencia - Tipo desconocido '%c' en linea: %s\n", type, line.c_str());
					break;
			}
		}
	}
	
	file.close();
	printf("CARGA: Antorchas: %zu - Llaves: %zu - Enemigo: %d - Salida: %d\n", ent.torches.size(), ent.keys.size(), ent.enemy.x_2D != -1, ent.exit.x_2D != -1);
	return ent;
}

/**
 * Función para crear el escenario del laberinto
 * Genera cubos para cada pared del mapa 2D
 * @return objeto con VAO y número de vértices para renderizar el escenario del laberinto
 */
objeto crear_escena(){
	objeto obj;
	GLuint VAO;
	GLuint buffer_pos, buffer_uv;
	
	// Creamos el laberinto con 15 filas, 15 columnas y tamaño de celda 4.0 unidades
	int side_size = 15;
	maze = new Maze(side_size, 4.0f);

	// Cargamos el mapa desde el archivo .txt (1 = muro, 0 = vacío)
	int* map = load_maze_from_file("bin/data/maze_map.txt", side_size);
	if (map == nullptr) {
		printf("CARGA: Error - No se pudo cargar el mapa del laberinto\n");
		return obj;
	}

	// Cargamos el mapa en la clase Maze
	maze->setMap(map, side_size);

	// Tamaño de cada celda del mapa
	float tile_size = maze->getTileSize(); 
	// Centro del laberinto en coordenadas XZ
	float maze_center_xz = (maze->getColumns() * tile_size) / 2.0f; // Simétrico en XZ

	// Cargamos los datos de antorchas
	entities = load_entities_from_file("bin/data/maze_map.txt", side_size, tile_size, maze_center_xz);

	// Contamos cuantos cubos necesitamos
	int wall_count = 0;
	for (int i = 0; i < maze->getRows(); i++){
		for(int j = 0; j < maze->getColumns(); j++){
			// Si el valor es 1, es un muro y necesitamos un cubo
			if (maze->getGrid(i, j) == 1){
				wall_count++;
			}
		}
	}

	// Cada cubo tiene 36 vértices (6 caras x 2 triángulos x 3 vértices)
	// Calculamos el número total de vértices necesarios para todas las paredes + 1 suelo
	int total_vertices = (wall_count + 1) * 36;

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

	// Para crear el suelo primero calculamos el tamaño total del laberinto
	float maze_wh = maze->getColumns() * tile_size * 2; // Simétrico en XZ
	float floor_thickness = 0.5f;
	
	// Calculamos la escala para que cubra todo el laberinto
	float scale_xz = maze_wh / 2.0f; // /2 porque va de -0.5 a 0.5
	float scale_y = floor_thickness / 2.0f;
	
	// Copiamos los vértices del cubo unitario, escalándolos y trasladándolos a su posición
	for (int k = 0; k < 36; k++){
		int v_index = vertex_index; // Índice para el vértice actual
		pos_data[v_index] = cube_vertices[k][0] * scale_xz;
		pos_data[v_index + 1] = cube_vertices[k][1] * scale_y;
		pos_data[v_index + 2] = cube_vertices[k][2] * scale_xz;
		vertex_index += 3;

		// Color del suelo: gris oscuro (0.4, 0.4, 0.4)
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

	// Mandamos posiciones en un VBO
	glGenBuffers(1, &buffer_pos); 
	glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 3 * sizeof(GLfloat), pos_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Mandamos colores en otro VBO
	glGenBuffers(1, &buffer_uv); 
	glBindBuffer(GL_ARRAY_BUFFER, buffer_uv);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 2 * sizeof(GLfloat), uv_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint buffer_norm;
	glGenBuffers(1, &buffer_norm); 
	glBindBuffer(GL_ARRAY_BUFFER, buffer_norm);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 3 * sizeof(GLfloat), normal_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLuint buffer_tan;
	glGenBuffers(1, &buffer_tan); 
	glBindBuffer(GL_ARRAY_BUFFER, buffer_tan);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 3 * sizeof(GLfloat), tangent_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Creamos y enlazamos el VAO
	glGenVertexArrays(1, &VAO);	
	glBindVertexArray(VAO);

	// Indicamos donde hallar datos de posiciones
	glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Indicamos donde hallar datos de colores
	glBindBuffer(GL_ARRAY_BUFFER, buffer_uv);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_norm);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, buffer_tan);
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
    
	escena_cubica = crear_escena();  // Crear el escenario del laberinto con todas las paredes

	// Habilitamos test de profundidad para renderizar correctamente las caras
	glEnable(GL_DEPTH_TEST);

	// Mandamos programas a GPU, compilar y crear programa en GPU

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
	GLuint tex_brick = cargar_textura("bin/data/brick.jpg", GL_TEXTURE0);
	transfer_int("tex", 0);

	GLuint tex_normal = cargar_textura("bin/data/brick_n.jpg", GL_TEXTURE1);
	transfer_int("normalMap", 1);

	GLuint tex_displacement = cargar_textura("bin/data/brick_d.jpg", GL_TEXTURE3);
	transfer_int("displacementMap", 2);

	GLuint tex_ao = cargar_textura("bin/data/brick_ao.jpg", GL_TEXTURE4);
	transfer_int("aoMap", 3);

	// Inicializamos módulos
	torch_module::init(); // Antorchas
	flame::init(); // Llamas de las antorchas
	particleSystem.init(); // Sistema de partículas

	// Volver al programa principal
	glUseProgram(prog);
	printf("DEBUG: numero de antorchas = %zu\n", entities.torches.size());
}


// Variables para la cámara
vec3 cam_pos = vec3(-26.0f, 3.0f, -26.0f); // Posición inicial de la cámara (observador)
vec3 cam_target = vec3(0.0f, 0.0f, 1.0f); // Dirección hacia donde mira la cámara
vec3 cam_up = vec3(0.0f, 1.0f, 0.0f); // Vector "arriba" de la cámara
int cam_fov = 62; // Campo de visión inicial
int cam_speed = 3; // Velocidad de movimiento de la cámara
int cam_run_speed = 4; // Velocidad en carrera (Shift)
float aspect_ratio = 4.0f / 3.0f; // Aspect ratio inicial (proporción de la ventana)
float cam_radius = 0.5f; // Radio de colisión de la cámara (como esfera)

// Variables para control con ratón
double last_mouse_x = 0.0; // Última posición X del ratón
double last_mouse_y = 0.0; // Última posición Y del ratón
float mouse_sensitivity = 0.1f; // Sensibilidad
float cam_yaw = 0.0f; // Rotación horizontal (grados)
float cam_pitch = 0.0f; // Rotación vertical (grados)

// Variables para control de teclado
bool keys_pressed[7] = {false, false, false, false, false, false, false}; // W, S, D, A, Q, E, Shift
double last_frame_time = 0.0; // Tiempo del último frame

// Variables de ImGui
bool show_stats = true;
bool show_settings = false;

// Variables para controlar la intensidad de los mapas de texturas
float displacement_intensity = 1.0f;  // Intensidad del desplazamiento (0.0 - 2.0)
float ao_intensity = 1.0f;  // Intensidad del ambient occlusion (0.0 - 2.0)

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

/**
 * Función para renderizar el panel de configuración
 */
void renderSettingsPanel() {
	// Posición del panel en centro de la pantalla y tamaño prefijado (siempre que aparezca)
	ImGui::SetNextWindowPos(ImVec2(ANCHO / 2.0f - 200, ALTO / 2.0f - 150), ImGuiCond_Appearing);
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);
    
	// Si se muestra el panel, renderizamos su contenido (sin movimiento ni colapso)
    if (show_settings && ImGui::Begin("Configuración", &show_settings, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
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
 * Función para renderizar la interfaz del juego con ImGui
 * @param io Referencia a la estructura i/o de ImGui
 */
void renderGameUI(ImGuiIO& io) {
	// Panel de estadísticas en esquina superior derecha, con tamaño fijo (siempre)
    ImGui::SetNextWindowPos(ImVec2(ANCHO - 320, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300, 160), ImGuiCond_Always);
    
	// Si se muestra el panel, renderizamos su contenido (sin título ni movimiento)
    if (show_stats && ImGui::Begin("Estadísticas", &show_stats, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
		ImGui::Text("Salud");
        ImGui::ProgressBar(player_health / 100.0f, ImVec2(-1, 0));
        
        ImGui::Text("Maná");
        ImGui::ProgressBar(player_mana / 50.0f, ImVec2(-1, 0));

		ImGui::Text("Llaves");
        ImGui::ProgressBar(player_keys / 3.0f, ImVec2(-1, 0));
        
        ImGui::End();
    }
    
    // Si se muestra el panel de configuración llamamos a su render
	if (show_settings){
		renderSettingsPanel();
	}
}

// Declaración adelantada de función
bool can_move(vec3& new_pos);

/**
 * Función para renderizar la escena en cada frame
 * Limpia buffers, actualiza matrices de tranformación, posición de cámara, luces...
 */
void render_scene()
{
	// Evitamos procesar si la ventana está minimizada
	if (ANCHO == 0 || ALTO == 0) {
		return;
	}
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Especifica color para el fondo oscuro (RGB+alfa)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpia buffers de color y profundidad

	//Construir array de luces
	lighting::clear();

	vec3 torchColor = vec3(1.0f, 0.7f, 0.35f);
	float tile_size = maze->getTileSize();
	float maze_center_xz = (maze->getColumns() * tile_size) / 2.0f;

	// Luz del jugador
	lighting::add(cam_pos + vec3(0.0f, -0.5f, 0.0f), torchColor);
	
	// Luz de las antorchas
	for(const Torch& t : entities.torches){
		lighting::add(t.lightPos, torchColor);
	}
	
	// Calculamos delta time para movimiento suave (e independiente de FPS)
	double current_time = glfwGetTime();
	double delta_time = current_time - last_frame_time;
	last_frame_time = current_time;

	// Establecemos la velocidad según si se está presionando Shift para correr o no
	float speed = keys_pressed[6] ? cam_run_speed : cam_speed;

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

	///////// Actualizacion matrices M, V, P  /////////	
	mat4 P, V, M, T, R, S;

	P = perspective(glm::radians((float)cam_fov), aspect_ratio, 0.1f, 50.0f);  // FOV, aspect ratio, Znear, Zfar
	
	// El target es un punto adelante de la cámara en la dirección donde mira
	vec3 target = cam_pos + cam_target;
	V = lookAt(cam_pos, target, cam_up);  // Pos camara, Lookat relativo a dirección, head up
	
	// Matriz identidad para el cubo (sin transformaciones)
	M = glm::mat4(1.0f);
	
	glUseProgram(prog);

	transfer_mat4("MVP", P * V * M);
	transfer_mat4("M", M);
	transfer_vec3("camPos", cam_pos);
	transfer_float("displacement_intensity", displacement_intensity);
	transfer_float("ao_intensity", ao_intensity);
	transfer_float("time", (float)current_time);
	
	// Subir array de luces
	lighting::upload_to_shader(prog);

	// Dibujamos el cubo del escenario
	glBindVertexArray(escena_cubica.VAO);             // Activamos VAO del cubo
	glDrawArrays(GL_TRIANGLES, 0, escena_cubica.Nv);  // Dibujamos todos los triangulos del cubo
	glBindVertexArray(0);                             // Desconectamos VAO

	// Dibujamos antorchas según entidades cargadas del mapa
	const float torch_scale = 0.6f;
	for(Torch& t : entities.torches){
		torch_module::draw(t.position, torch_scale, t.rot_y, P, V);

		// La llama va por encima del torch + adelante (en la dirección del pasillo)
		flame::draw(t.flamePos, 2.0f, P, V);

		// Actualizamos las partículas de la antorcha
		t.particleEmitter.update(delta_time, t.flamePos, particleSystem);
	}

	// Actualizamos sistema de partículas
	particleSystem.update((float)delta_time);

	// Dibujamos partículas
	particleSystem.render(P, V);

	// Obtenemos referencia a ImGuiIO
	ImGuiIO& io = ImGui::GetIO();

	// Renderizamos la interfaz de usuario con ImGui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	renderGameUI(io);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	// Mostramos/ocultamos el cursor cuando hay menú abierto
	if (show_settings) {
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     ASIGNACIÓN FUNCIONES CALLBACK
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
	if (show_settings) {
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
 */
void asigna_funciones_callback(GLFWwindow* window)
{
	glfwSetWindowSizeCallback(window, ResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
}
