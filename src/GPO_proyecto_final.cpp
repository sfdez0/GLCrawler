/**
 * Proyecto final de GpO
 * Título: Dungeon Crawler 3D
 * ATG, 2019
 */

#include <GpO.h>
#include <vector>
#include <maze.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     VARIABLES GLOBALES 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tamaño inicial ventana
int ANCHO = 800, ALTO = 600; 

// Título de la ventana
const char* prac = "OpenGL (GpO)"; 

// Puntero a la clase Maze que representa el mapa del laberinto
Maze* maze; 


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

	uniform mat4 MVP = mat4(1.0f); // Matriz de transformación MVP
	uniform mat4 M = mat4(1.0f);

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
	uniform vec3 lightPos;
	uniform vec3 camPos;

	out vec3 outputColor; // Color final que se pintará en la pantalla
	void main() {
		vec3 texColor = texture(tex, UV).rgb;
		vec3 torchColor = vec3(1.0, 0.9, 0.75);

		vec3 N = texture(normalMap, UV).rgb;
		N = N * 2.0 - 1.0;
		N.xy *= 2.5;
		N = normalize(TBN * N);

		float ambient = 0.05;

		vec3 L = normalize(lightPos - FragPos);
		float diffuse = max(dot(N,L), 0.0);

		vec3 V = normalize(camPos - FragPos);
		vec3 R = reflect(-L, N);
		float specular = pow(max(dot(V, R), 0.0), 4.0) * 0.15;

		float dist = length(lightPos - FragPos);
		float attenuation = 1.0 / (0.5 + 0.05 * dist + 0.02 * dist * dist);

		outputColor = (texColor * (ambient + 1.5 * diffuse * attenuation) + vec3(specular * attenuation)) * torchColor ;
	}
);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE AND DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLFWwindow* window;
GLuint prog;
objeto escena_cubica; // Objeto para el escenario del laberinto

/**
 * Función para crear el escenario del laberinto
 * Genera cubos para cada pared del mapa 2D
 */
objeto crear_escena(){
	objeto obj;
	GLuint VAO;
	GLuint buffer_pos, buffer_uv;
	
	// Creamos el laberinto con 15 filas, 15 columnas y tamaño de celda 4.0 unidades
	maze = new Maze(15, 4.0f);

	// (1 = muro, 0 = vacío)
	int map[] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1,
        1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1,
        1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1,
        1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1,
        1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1,
        1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 1,
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
        1, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1,
        1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1,
		1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 1,
		1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 0, 1,
		1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 
		1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	};

	// Cargamos el mapa en la clase Maze
	maze->setMap(map, 15);

	// Contamos cuantos cubos necesitamos
	int wall_count = 0;
	for (int i = 0; i < maze->getRows(); i++){
		for(int j = 0; j < maze->getColumns(); j++){
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
	float tile_size = maze->getTileSize(); // Tamaño de cada celda del mapa
	float maze_center_xz = (maze->getColumns() * tile_size) / 2.0f; // Simétrico en XZ
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
void init_scene()
{
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
	GLuint tex_brick = cargar_textura("data/brick.jpg", GL_TEXTURE0);
	transfer_int("tex", 0);

	GLuint tex_normal = cargar_textura("data/brick_normal.jpg", GL_TEXTURE1);
	transfer_int("normalMap",1);
}


// Variables para la cámara
vec3 cam_pos = vec3(0.0f, 3.0f, 3.0f); // Posición inicial de la cámara (observador)
vec3 cam_target = vec3(0.0f, 0.0f, 1.0f); // Dirección hacia donde mira la cámara
vec3 cam_up = vec3(0.0f, 1.0f, 0.0f); // Vector "arriba" de la cámara
float cam_fov = 60.0f; // Campo de visión inicial
float cam_speed = 3.0f; // Velocidad de movimiento de la cámara
float cam_run_speed = 4.5f; // Velocidad en carrera (Shift)
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

// Declaración adelantada de función
bool can_move(vec3& new_pos);

/**
 * Función para renderizar la escena en cada frame
 * Limpia buffers, actualiza matrices de tranformación, posición de cámara, luces...
 */
void render_scene()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Especifica color para el fondo oscuro (RGB+alfa)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Limpia buffers de color y profundidad

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

	P = perspective(glm::radians(cam_fov), aspect_ratio, 0.1f, 50.0f);  // FOV, aspect ratio, Znear, Zfar
	
	// El target es un punto adelante de la cámara en la dirección donde mira
	vec3 target = cam_pos + cam_target;
	V = lookAt(cam_pos, target, cam_up);  // Pos camara, Lookat relativo a dirección, head up
	
	// Matriz identidad para el cubo (sin transformaciones)
	M = glm::mat4(1.0f);
	
	transfer_mat4("MVP", P * V * M);
	transfer_mat4("M", M);
	transfer_vec3("lightPos", cam_pos + vec3(0.0f, -1.5f, 0.0f));
	transfer_vec3("camPos", cam_pos);
	
	// Dibujamos el cubo del escenario
	glBindVertexArray(escena_cubica.VAO);             // Activamos VAO del cubo
	glDrawArrays(GL_TRIANGLES, 0, escena_cubica.Nv);  // Dibujamos todos los triangulos del cubo
	glBindVertexArray(0);                             // Desconectamos VAO
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     PROGRAMA PRINCIPAL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Programa principal
 */
int main(int argc, char* argv[])
{
	init_GLFW();            // Inicializa lib GLFW
	window = Init_Window(prac);  // Crea ventana usando GLFW, asociada a un contexto OpenGL	X.Y
	load_Opengl();         // Carga funciones de OpenGL, comprueba versión.
	init_scene();          // Prepara escena
	asigna_funciones_callback(window); // Registramos callbacks de entrada
	
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
 * `window`: puntero a la ventana GLFW
 * `width`, `height`: nuevo ancho y alto de la ventana
 */
void ResizeCallback(GLFWwindow* window, int width, int height)
{
	glfwGetFramebufferSize(window, &width, &height); 
	glViewport(0, 0, width, height);
	ALTO = height;	
	ANCHO = width;

	// Actualizamos aspect ratio para la matriz de proyeccion
	aspect_ratio = (float)ANCHO / (float)ALTO; 

	// Actualizamos fov proporcionalmente al cambio de aspect ratio
	cam_fov = 60.0f * (4.0f / 3.0f) / aspect_ratio;
}

/**
 * Función que procesa las pulsaciones de teclas para controlar el movimiento de la cámara
 * `window`: puntero a la ventana GLFW
 * `key`: código de la tecla pulsada
 * `code`: código
 * `action`: tipo de acción
 * `mode`: estado de las teclas modificadoras (Shift, Ctrl, Alt)
 */
static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}

	// Actualizamos el estado de las teclas de movimiento
	bool is_pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
	
	switch (key) {
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
 * `window`: puntero a la ventana GLFW
 * `xpos`, `ypos`: posición actual del cursor en la ventana
 */
static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
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
