/************************  GPO_01 ************************************
ATG, 2019
******************************************************************************/

#include <GpO.h>
#include <vector>

// TAMAÑO y TITULO INICIAL de la VENTANA
int ANCHO = 800, ALTO = 600;  // Tamaño inicial ventana
const char* prac = "OpenGL (GpO)"; // Nombre de la practica (aparecera en el titulo de la ventana).

// Estructura para Bounding Box
struct BoundingBox {
	vec3 min;
	vec3 max;
};

// Vector de bounding boxes para colisiones
std::vector<BoundingBox> collision_boxes;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 330 core\n" #src

// Vertex Shader (in: pos, color; out: col; uniform: MVP)
const char* vertex_prog = GLSL(
	layout(location = 0) in vec3 pos; // Posición del vértice
	layout(location = 1) in vec3 color; // Color del vértice
	out vec3 col; // Color que se pasará al fragment shader
	uniform mat4 MVP = mat4(1.0f); // Matriz de transformación MVP
	void main() { 
		gl_Position = MVP * vec4(pos,1); // Construimos coordenadas homogéneas y aplicamos matriz MVP
		col = color; // Pasamos el color al fragment shader
	}
);

// Fragment Shader (in: col; out: outputColor)
const char* fragment_prog = GLSL(
	in vec3 col; // Color recibido del vertex shader
	out vec3 outputColor; // Color final que se pintará en la pantalla
	void main() {
		outputColor = col;
	}
);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////   RENDER CODE AND DATA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

GLFWwindow* window;
GLuint prog;
objeto escena_cubica; // Objeto para el escenario del laberinto

// Función que crea un rectángulo 3D (escenario del laberinto)
// Dimensiones: Ancho(X) 10, Alto(Y) 4, Profundidad(Z) 20
objeto crear_escena_cubica(void)
{
	objeto obj;
	GLuint VAO;
	GLuint buffer_pos, buffer_col;

	// Definimos los 36 vértices del cubo (6 caras x 2 tríangulos x 3 vértices)
	// Dimensiones: X(-5 a 5), Y(-2 a 2), Z(-10 a 10)
	GLfloat pos_data[36][3] = {
		// Cara frontal (z = 10) - Rojo
		-5.0f, -2.0f,  10.0f,   // tri 1: inf-izq
		 5.0f, -2.0f,  10.0f,   //        inf-der
		 5.0f,  2.0f,  10.0f,   //        sup-der
		-5.0f, -2.0f,  10.0f,   // tri 2: inf-izq
		 5.0f,  2.0f,  10.0f,   //        sup-der
		-5.0f,  2.0f,  10.0f,   //        sup-izq

		// Cara trasera (z = -10) - Verde
		 5.0f, -2.0f, -10.0f,   // tri 1: inf-der
		-5.0f, -2.0f, -10.0f,   //        inf-izq
		-5.0f,  2.0f, -10.0f,   //        sup-izq
		 5.0f, -2.0f, -10.0f,   // tri 2: inf-der
		-5.0f,  2.0f, -10.0f,   //        sup-izq
		 5.0f,  2.0f, -10.0f,   //        sup-der

		// Cara derecha (x = 5) - 2 Azul
		 5.0f, -2.0f,  10.0f,   // tri 1: inf-der
		 5.0f, -2.0f, -10.0f,   //        inf-izq
		 5.0f,  2.0f, -10.0f,   //        sup-izq
		 5.0f, -2.0f,  10.0f,   // tri 2: inf-der
		 5.0f,  2.0f, -10.0f,   //        sup-izq
		 5.0f,  2.0f,  10.0f,   //        sup-der

		// Cara izquierda (x = -5) - Amarillo
		-5.0f, -2.0f, -10.0f,   // tri 1: inf-der
		-5.0f, -2.0f,  10.0f,   //        inf-izq
		-5.0f,  2.0f,  10.0f,   //        sup-izq
		-5.0f, -2.0f, -10.0f,   // tri 2: inf-der
		-5.0f,  2.0f,  10.0f,   //        sup-izq
		-5.0f,  2.0f, -10.0f,   //        sup-der

		// Cara superior (y = 2) - Cian
		-5.0f,  2.0f,  10.0f,   // tri 1: izq-atras
		 5.0f,  2.0f,  10.0f,   //        der-atras
		 5.0f,  2.0f, -10.0f,   //        der-frente
		-5.0f,  2.0f,  10.0f,   // tri 2: izq-atras
		 5.0f,  2.0f, -10.0f,   //        der-frente
		-5.0f,  2.0f, -10.0f,   //        izq-frente

		// Cara inferior (y = -2) - Magenta
		-5.0f, -2.0f, -10.0f,   // tri 1: izq-frente
		 5.0f, -2.0f, -10.0f,   //        der-frente
		 5.0f, -2.0f,  10.0f,   //        der-atras
		-5.0f, -2.0f, -10.0f,   // tri 2: izq-frente
		 5.0f, -2.0f,  10.0f,   //        der-atras
		-5.0f, -2.0f,  10.0f    //        izq-atras
	};

	// Colores para cada cara
	GLfloat color_data[36][3];
	
	// Definimos los colores para cada cara
	GLfloat colors[6][3] = {
		{1.0f, 0.0f, 0.0f},  // Rojo (frontal)
		{0.0f, 1.0f, 0.0f},  // Verde (trasera)
		{0.0f, 0.0f, 1.0f},  // Azul (derecha)
		{1.0f, 1.0f, 0.0f},  // Amarillo (izquierda)
		{0.0f, 1.0f, 1.0f},  // Cian (superior/techo)
		{1.0f, 0.0f, 1.0f}   // Magenta (inferior/suelo)
	};
	
	// Bucle para asignar colores a todos los vértices
	for (int i = 0; i < 36; i++) {
		// Determinamos a que cara pertenece el vértice
		int face = i / 6; 

		// Aplicamos el color RGB
		color_data[i][0] = colors[face][0];
		color_data[i][1] = colors[face][1];
		color_data[i][2] = colors[face][2];
	}

	// Mandamos posiciones en un VBO
	glGenBuffers(1, &buffer_pos); 
	glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pos_data), pos_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Mandamos colores en otro VBO
	glGenBuffers(1, &buffer_col); 
	glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
	glBufferData(GL_ARRAY_BUFFER, sizeof(color_data), color_data, GL_STATIC_DRAW);
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
	glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Cerramos VAO con todo listo para ser pintado
	glBindVertexArray(0);

	// Devolvemos objeto VAO + número de vértices en estructura obj
	obj.VAO = VAO; 
	obj.Nv = 36;
	return obj;
}

// Preparacion de los datos de los objetos a dibujar, envialarlos a la GPU
// Compilacion programas a ejecutar en la tarjeta grafica:  vertex shader, fragment shaders
// Opciones generales de render de OpenGL
void init_scene()
{
	int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height); 
    
	escena_cubica = crear_escena_cubica();  // Crear el escenario cubico

	// Inicializamos el vector de bounding boxes de colisión
	collision_boxes.clear();
	
	// Creamos las bounding boxes para cada pared del cubo
	// Pared frontal (z = 10)
	collision_boxes.push_back({vec3(-5.0f, -2.0f, 9.5f), vec3(5.0f, 2.0f, 10.0f)});
	// Pared trasera (z = -10)
	collision_boxes.push_back({vec3(-5.0f, -2.0f, -10.0f), vec3(5.0f, 2.0f, -9.5f)});
	// Pared derecha (x = 5)
	collision_boxes.push_back({vec3(4.5f, -2.0f, -10.0f), vec3(5.0f, 2.0f, 10.0f)});
	// Pared izquierda (x = -5)
	collision_boxes.push_back({vec3(-5.0f, -2.0f, -10.0f), vec3(-4.5f, 2.0f, 10.0f)});
	// Techo (y = 2)
	collision_boxes.push_back({vec3(-5.0f, 1.5f, -10.0f), vec3(5.0f, 2.0f, 10.0f)});
	// Suelo (y = -2)
	collision_boxes.push_back({vec3(-5.0f, -2.0f, -10.0f), vec3(5.0f, -1.5f, 10.0f)});

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
}


// Variables para la cámara
vec3 cam_pos = vec3(0.0f, 0.0f, 0.0f); // Posición inicial de la cámara (observador)
vec3 cam_target = vec3(0.0f, 0.0f, 1.0f); // Dirección hacia donde mira la cámara
vec3 cam_up = vec3(0.0f, 1.0f, 0.0f); // Vector "arriba" de la cámara
float cam_fov = 60.0f; // Campo de visión inicial
float cam_speed = 0.2f; // Velocidad de movimiento de la cámara
float cam_collision_radius = 0.5f; // Radio de colisión de la cámara
float aspect_ratio = 4.0f / 3.0f; // Aspect ratio inicial (proporción de la ventana)

// Actualizar escena: cambiar posicion objetos, nuevos objetros, posicion camara, luces, etc.
void render_scene()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Especifica color para el fondo oscuro (RGB+alfa)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Limpia buffers de color y profundidad

	float t = (float)glfwGetTime();  // Contador de tiempo en segundos 

	///////// Actualizacion matrices M, V, P  /////////	
	mat4 P, V, M, T, R, S;

	P = perspective(glm::radians(cam_fov), aspect_ratio, 0.1f, 50.0f);  // FOV, aspect ratio, Znear, Zfar
	
	// El target es un punto adelante de la cámara en la dirección donde mira
	vec3 target = cam_pos + cam_target;
	V = lookAt(cam_pos, target, cam_up);  // Pos camara, Lookat relativo a dirección, head up
	
	// Matriz identidad para el cubo (sin transformaciones)
	M = glm::mat4(1.0f);
	
	transfer_mat4("MVP", P * V * M);
	
	// Dibujamos el cubo del escenario
	glBindVertexArray(escena_cubica.VAO);             // Activamos VAO del cubo
	glDrawArrays(GL_TRIANGLES, 0, escena_cubica.Nv);  // Dibujamos todos los triangulos del cubo
	glBindVertexArray(0);                             // Desconectamos VAO
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PROGRAMA PRINCIPAL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Programa principal
int main(int argc, char* argv[])
{
	init_GLFW();            // Inicializa lib GLFW
	window = Init_Window(prac);  // Crea ventana usando GLFW, asociada a un contexto OpenGL	X.Y
	load_Opengl();         // Carga funciones de OpenGL, comprueba versión.
	init_scene();          // Prepara escena
	
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

// Función para mostrar información en el título de la ventana
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

// Función para limitar un valor entre un mínimo y un máximo
float clamp(float value, float min, float max) {
	return glm::max(min, glm::min(value, max));
}

// Función para calcular si existe colision entre una esfera y una bounding box
bool sphere_aabb_collision(vec3& sphere_center, float& radius, const BoundingBox& box) {
	// Buscamos el punto más cercano en la caja al centro de la esfera
	vec3 closest;
	// Por cada eje, comprobamos si el centro está dentro de los límites de la caja y ajustamos el punto más cercano en consecuencia
	closest.x = clamp(sphere_center.x, box.min.x, box.max.x);
	closest.y = clamp(sphere_center.y, box.min.y, box.max.y);
	closest.z = clamp(sphere_center.z, box.min.z, box.max.z);

	// Calculamos la distancia entre el centro de la esfera y el punto más cercano de la caja
	float distance = glm::length(sphere_center - closest);
	return distance < radius;
}

// Función para verificar si hay colisiones entre una esfera y una bounding box
bool check_movement_collision(vec3& new_pos, float& radius) {
	// Iteramos por todas las bounding boxes de colisión
	for (const BoundingBox& box : collision_boxes) {
		// Comprobamos si la esfera colisiona con la caja
		if (sphere_aabb_collision(new_pos, radius, box)) {
			fprintf(stdout, "Collision detected\n");
			return true;
		}
	}

	return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////  ASIGNACIÓN FUNCIONES CALLBACK
///////////////////////////////////////////////////////////////////////////////////////////////////////////

// Callback de cambio tamaño de ventana
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

// Callback de pulsacion de tecla
static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
	fprintf(stdout, "Key %d Code %d Act %d Mode %d Pos (%f, %f, %f)\n", key, code, action, mode, cam_pos.x, cam_pos.y, cam_pos.z);
	
	if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}
	// Movimiento hacia adelante (W)
	else if (key == GLFW_KEY_W){
		vec3 move = glm::normalize(vec3(cam_target.x, 0.0f, cam_target.z)) * cam_speed;
		vec3 new_pos = cam_pos + move;
		if (!check_movement_collision(new_pos, cam_collision_radius)) {
			cam_pos = new_pos;
		}
	}
	// Movimiento hacia atrás (S)
	else if (key == GLFW_KEY_S){
		vec3 move = glm::normalize(vec3(cam_target.x, 0.0f, cam_target.z)) * cam_speed;
		vec3 new_pos = cam_pos - move;
		if (!check_movement_collision(new_pos, cam_collision_radius)) {
			cam_pos = new_pos;
		}
	}
	// Movimiento hacia la derecha (D)
	else if (key == GLFW_KEY_D){
		vec3 right = glm::normalize(glm::cross(cam_target, cam_up));
		vec3 new_pos = cam_pos + right * cam_speed;
		if (!check_movement_collision(new_pos, cam_collision_radius)) {
			cam_pos = new_pos;
		}
	}
	// Movimiento hacia la izquierda (A)
	else if (key == GLFW_KEY_A){
		vec3 right = glm::normalize(glm::cross(cam_target, cam_up));
		vec3 new_pos = cam_pos - right * cam_speed;
		if (!check_movement_collision(new_pos, cam_collision_radius)) {
			cam_pos = new_pos;
		}
	}
	// Movimiento hacia arriba (Q)
	else if (key == GLFW_KEY_Q){
		vec3 new_pos = cam_pos + cam_up * cam_speed;
		if (!check_movement_collision(new_pos, cam_collision_radius)) {
			cam_pos = new_pos;
		}
	}
	// Movimiento hacia abajo (E)
	else if (key == GLFW_KEY_E){
		vec3 new_pos = cam_pos - cam_up * cam_speed;
		if (!check_movement_collision(new_pos, cam_collision_radius)) {
			cam_pos = new_pos;
		}
	}
	// Rotación hacia la izquierda (Z)
	else if (key == GLFW_KEY_Z){
		// Rotamos el vector front alrededor del eje Y (up) para girar la cámara a la izquierda
		float angle = glm::radians(-5.0f); // 5 grados antiohorario
		float cos_a = cos(angle);
		float sin_a = sin(angle);
		cam_target.x = cam_target.x * cos_a - cam_target.z * sin_a;
		cam_target.z = cam_target.x * sin_a + cam_target.z * cos_a;
	}
	// Rotación hacia la derecha (X)
	else if (key == GLFW_KEY_X){
		// Rotamos el vector front alrededor del eje Y (up) para girar la cámara a la derecha
		float angle = glm::radians(5.0f); // 5 grados
		float cos_a = cos(angle);
		float sin_a = sin(angle);
		cam_target.x = cam_target.x * cos_a - cam_target.z * sin_a;
		cam_target.z = cam_target.x * sin_a + cam_target.z * cos_a;
	}
}

void asigna_funciones_callback(GLFWwindow* window)
{
	glfwSetWindowSizeCallback(window, ResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
}



