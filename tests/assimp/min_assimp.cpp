#include "GPO_assimp_aux.h"

//Configuracion de la ventana
GLFWwindow* window;
int ANCHO = 800, ALTO = 600;
const char* prac = "OpenGL(GpO) Iluminacion";

//Variables para GL
GLuint bayer, blanco;
GLuint prog;

bool useTextures = true;

mat4 Proy, View, M;
vec3 campos=vec3(0.0f,1.0f,4.0f);
vec3 target=vec3(0.0f,0.9f,0.0f);
vec3 up = vec3(0, 1, 0);

//Escenas
struct escena miEscena;
struct escena* escenaActual = &miEscena;

//Codigo de los shaders
//////////////////////////////////////////////////////////////////////////////////
#define GLSL(src) "#version 330 core\n" #src

const char* vertex_prog1 = GLSL(
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
out float ilu;

uniform mat4 M;
uniform mat4 PV;
uniform vec3 luz = vec3(1, 1, 0) / sqrt(2.0f);

void main() {
	gl_Position = PV*M*vec4(pos, 1);

	mat3 M_adj = mat3(transpose(inverse(M)));
	vec3 n = M_adj * normal;

	vec3 nn = normalize(n);
	float difusa = dot(luz,nn); if (difusa < 0) difusa = 0; 
	ilu = (0.15 + 0.85*difusa);  //15% Ambiente + 85% difusa
}
);

const char* fragment_prog1 = GLSL(
in float ilu;
out vec3 col;
void main()
{
	col = vec3(1, 1, 0.9);
	col = col*ilu;
}
);

///////////////////////////////////////////////////////////////////////////////////////////////////

// Fucniones para el proyecto
void dibujar_indexado(objeto obj)
{
  glBindVertexArray(obj.VAO);
  glDrawElements(GL_TRIANGLES,obj.Ni,obj.tipo_indice,(void*)0);
  glBindVertexArray(0);
}

void dibujar_escena() {
	for (unsigned int i = 0; i < escenaActual->nInstancias; i++) {
		const unsigned int j = escenaActual->instIdx[i];
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, useTextures ? escenaActual->mats[j] : blanco);
		dibujar_indexado(escenaActual->objs[j]);
	}
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void init_scene()
{
	Proy = glm::perspective(glm::radians(55.0f), 4.0f / 3.0f, 0.1f, 100.0f);
	View = glm::lookAt(campos,target,up);

	prog = Compile_Link_Shaders(vertex_prog1, fragment_prog1); // Compile shaders prog1
	glUseProgram(prog);

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	miEscena = cargar_modelo_assimp("./data/suzanne.obj");

	/*
	//No usado por ahora, pero preparado para siguiente version
	bayer = cargar_textura("./data/bayer16.png", GL_TEXTURE0);
	//bayer = cargar_textura("./data/cottage_obj.png", GL_TEXTURE0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &blanco);
	glBindTexture(GL_TEXTURE_2D, blanco);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, (const GLubyte[]) {255, 255, 255});
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);*/

}

void render_scene()
{
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float tt = (float)glfwGetTime();

	//Generación de la matriz de modelado
	vec3 xy=vec3(0.0f, 0.5f, 0.0f);
	vec3 sc=vec3(2.0f, 2.0f, 2.0f);
	M = scale(sc)*translate(xy)*rotate(tt, vec3(0.0f, 1.0f, 0.0f));

	//Transferencia a la GPU
	transfer_mat4("PV",Proy*View); transfer_mat4("M", M);

	//Dibujado
	dibujar_escena();
}

int main(int argc, char* argv[])
{
	init_GLFW();
	window = Init_Window(prac);
	load_Opengl();
	init_scene();

	while (!glfwWindowShouldClose(window))
	{
		render_scene();  
		glfwSwapBuffers(window); glfwPollEvents();
		show_info();
	}

	limpiar_escena(&miEscena);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void show_info()
{
	static int fps = 0;
	static double last_tt = 0;
	double elapsed, tt;
	char nombre_ventana[128];

	fps++; tt = glfwGetTime();

	elapsed = (tt - last_tt);
	if (elapsed >= 0.5)
	{
		sprintf_s(nombre_ventana, 128, "%s: %4.0f FPS @ %d x %d", prac, fps / elapsed, ANCHO, ALTO);
		glfwSetWindowTitle(window, nombre_ventana);
		last_tt = tt; fps = 0;
	}
}

void ResizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	ALTO = height;
	ANCHO = width;
}

static void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{	
	switch (key)
	{
	 case GLFW_KEY_ESCAPE:	glfwSetWindowShouldClose(window, true); break;	
	}
}

void asigna_funciones_callback(GLFWwindow* window)
{
	glfwSetWindowSizeCallback(window, ResizeCallback);
	glfwSetKeyCallback(window, KeyCallback);
}
