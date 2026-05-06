#pragma once
#include "GpO.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct escena {
	unsigned int nObjetos;
	objeto* objs;
	GLuint* mats;
	vec3* diffuse;
	struct MeshBuffers* buffers;
	unsigned int nInstancias;
	unsigned int* instIdx;
};

struct MeshBuffers {
	GLuint pos;
	GLuint norm;
	GLuint uv;
	GLuint idx;
};

struct escena cargar_modelo_assimp(const char* file);
void limpiar_escena(struct escena* escena);
