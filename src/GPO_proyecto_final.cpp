/************************  GPO_01 ************************************
ATG, 2019
******************************************************************************/

#include <GpO.h>

// TAMA�O y TITULO INICIAL de la VENTANA
int ANCHO = 800, ALTO = 600;  // Tama�o inicial ventana
const char* prac = "OpenGL (GpO)";   // Nombre de la practica (aparecera en el titulo de la ventana).


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////     CODIGO SHADERS 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GLSL(src) "#version 330 core\n" #src

const char* vertex_prog = GLSL(
layout(location = 0) in vec3 pos; 
layout(location = 1) in vec3 color;
out vec3 col;
uniform mat4 MVP=mat4(1.0f);
void main()
 {
  gl_Position = MVP*vec4(pos,1); // Construyo coord homog�neas y aplico matriz transformacion M
  col = color;                             // Paso color a fragment shader
 }
);

const char* fragment_prog = GLSL(
in vec3 col;
out vec3 outputColor;
void main() 
 {
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
	glGenBuffers(1, &buffer_pos); glBindBuffer(GL_ARRAY_BUFFER, buffer_pos);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pos_data), pos_data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Mandamos colores en otro VBO
	glGenBuffers(1, &buffer_col); glBindBuffer(GL_ARRAY_BUFFER, buffer_col);
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


vec3 pos_obs = vec3(0.0f, 0.0f, 0.0f); // Posición inicial de la cámara (observador)
vec3 front = vec3(0.0f, 0.0f, 1.0f); // Dirección hacia donde mira la cámara
vec3 up = vec3(0.0f, 1.0f, 0.0f); // Vector "arriba" de la cámara

float fov = 60.0f; // Campo de visión inicial
float aspect = 4.0f / 3.0f; // Aspect ratio inicial (proporción de la ventana)
float speed = 0.2f;  // Velocidad de movimiento de la cámara

// Actualizar escena: cambiar posicion objetos, nuevos objetros, posicion camara, luces, etc.
void render_scene()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Especifica color para el fondo oscuro (RGB+alfa)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // Limpia buffers de color y profundidad

	float t = (float)glfwGetTime();  // Contador de tiempo en segundos 

	///////// Actualizacion matrices M, V, P  /////////	
	mat4 P, V, M, T, R, S;

	P = perspective(glm::radians(fov), aspect, 0.1f, 50.0f);  // FOV, aspect ratio, Znear, Zfar
	
	// El target es un punto adelante de la cámara en la dirección donde mira
	vec3 target = pos_obs + front;
	V = lookAt(pos_obs, target, up);  // Pos camara, Lookat relativo a dirección, head up
	
	// Matriz identidad para el cubo (sin transformaciones)
	M = glm::mat4(1.0f);
	
	transfer_mat4("MVP", P * V * M);
	
	// Dibujamos el cubo del escenario
	glBindVertexArray(escena_cubica.VAO);             // Activamos VAO del cubo
	glDrawArrays(GL_TRIANGLES, 0, escena_cubica.Nv);  // Dibujamos todos los triangulos del cubo
	glBindVertexArray(0);                             // Desconectamos VAO

	////////////////////////////////////////////////////////
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PROGRAMA PRINCIPAL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	init_GLFW();            // Inicializa lib GLFW
	window = Init_Window(prac);  // Crea ventana usando GLFW, asociada a un contexto OpenGL	X.Y
	load_Opengl();         // Carga funciones de OpenGL, comprueba versi�n.
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


//////////  FUNCION PARA MOSTRAR INFO OPCIONAL EN EL TITULO DE VENTANA  //////////
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////  ASIGNACON FUNCIONES CALLBACK
///////////////////////////////////////////////////////////////////////////////////////////////////////////


// Callback de cambio tama�o de ventana
void ResizeCallback(GLFWwindow* window, int width, int height)
{
	glfwGetFramebufferSize(window, &width, &height); 
	glViewport(0, 0, width, height);
	ALTO = height;	ANCHO = width;
}

// Callback de pulsacion de tecla
static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
	fprintf(stdout, "Key %d Code %d Act %d Mode %d\n", key, code, action, mode);
	if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
}


void asigna_funciones_callback(GLFWwindow* window)
{
	glfwSetWindowSizeCallback(window, ResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
}



