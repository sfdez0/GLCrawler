#include "GPO_assimp_aux.h"

GLuint build_VBO(void* data, GLsizeiptr size, GLenum type) {
	GLuint buffer;
	//Creamos un nuevo VBO en la GPU y copiamos los datos
	glGenBuffers(1, &buffer);
	glBindBuffer(type, buffer);
	glBufferData(type, size, data, GL_STATIC_DRAW);
	return buffer;
}

GLuint build_VAO(aiMesh* mesh, MeshBuffers* out_buffers) {
	//TODO: Colores de los vertices, UVs (quizás materiales)
	float* vertices = new float[mesh->mNumVertices*3];
	float* normales = new float[mesh->mNumVertices*3];
	float* uvs = new float[mesh->mNumVertices*2];
	unsigned int* indices = new unsigned int[mesh->mNumFaces*3];

	//Copia de las coordenadas, normales y UVs de cada vértice
	for (int i = 0; i < mesh->mNumVertices; i++) {
		const int idx = i*3;
		vertices[idx] = mesh->mVertices[i].x;
		vertices[idx+1] = mesh->mVertices[i].y;
		vertices[idx+2] = mesh->mVertices[i].z;
	}
	for (int i = 0; i < mesh->mNumVertices; i++) {
		const int idx = i*3;
		normales[idx] = mesh->mNormals[i].x;
		normales[idx+1] = mesh->mNormals[i].y;
		normales[idx+2] = mesh->mNormals[i].z;
	}
	for (int i = 0; i < mesh->mNumVertices; i++) {
		const int idx = i*2;
		uvs[idx] = mesh->mTextureCoords[0][i].x;
		uvs[idx+1] = mesh->mTextureCoords[0][i].y;
	}
	//Copia de los índices, para dibujado con índices
	for (int i = 0; i < mesh->mNumFaces; i++) {
		const int idx = i*3;
		indices[idx] = mesh->mFaces[i].mIndices[0];
		indices[idx+1] = mesh->mFaces[i].mIndices[1];
		indices[idx+2] = mesh->mFaces[i].mIndices[2];
	}

	//Creamos un VAO
	GLuint buffer;
	glGenVertexArrays(1, &buffer);
	glBindVertexArray(buffer);

	//Creamos VBO con coordenadas de cada vértice y lo asignamos a layout 0
	if (out_buffers) {
		out_buffers->pos = build_VBO(vertices, mesh->mNumVertices*sizeof(float)*3, GL_ARRAY_BUFFER);
	} else {
		build_VBO(vertices, mesh->mNumVertices*sizeof(float)*3, GL_ARRAY_BUFFER);
	}
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

	//Creamos un VBO con normales de cada vértice y lo asignamos a layout 1
	if (out_buffers) {
		out_buffers->norm = build_VBO(normales, mesh->mNumVertices*sizeof(float)*3, GL_ARRAY_BUFFER);
	} else {
		build_VBO(normales, mesh->mNumVertices*sizeof(float)*3, GL_ARRAY_BUFFER);
	}
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);

	//Creamos un VBO con UVs de cada vértice y lo asignamos a layout 2
	if (out_buffers) {
		out_buffers->uv = build_VBO(uvs, mesh->mNumVertices*sizeof(float)*2, GL_ARRAY_BUFFER);
	} else {
		build_VBO(uvs, mesh->mNumVertices*sizeof(float)*2, GL_ARRAY_BUFFER);
	}
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);

	//Creamos un VBO con los índices
	if (out_buffers) {
		out_buffers->idx = build_VBO(indices, mesh->mNumFaces*sizeof(unsigned int)*3, GL_ELEMENT_ARRAY_BUFFER);
	} else {
		build_VBO(indices, mesh->mNumFaces*sizeof(unsigned int)*3, GL_ELEMENT_ARRAY_BUFFER);
	}

	//Limpieza y un-bind de todos los buffers
	delete []vertices;
	delete []normales;
	delete []uvs;
	delete []indices;
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return buffer;
}

objeto build_objeto(aiMesh* mesh, MeshBuffers* out_buffers) {
	objeto obj;
	obj.VAO = build_VAO(mesh, out_buffers);
	//TODO: Debería de haber una forma de saber el tamaño en bytes de los índices, para evitar malgastar memoria
	obj.tipo_indice = GL_UNSIGNED_INT;
	obj.Ni = mesh->mNumFaces*3;
	obj.Nv = mesh->mNumVertices*3;
	return obj;
}

GLuint build_material(aiMaterial* mat) {
	aiString str;
	/*for (unsigned int j = 0; j < mat->mNumProperties; j++) {
	 *	str = mat->mProperties[j]->mKey;
	 *	printf("\t\tProp%u: %s\n", j, str.C_Str());
	}*/
	mat->Get(_AI_MATKEY_TEXTURE_BASE, aiTextureType_DIFFUSE, 0, str);
	return cargar_textura(str.C_Str(), GL_TEXTURE0);
}

struct escena cargar_modelo_assimp(const char* file) {
	struct escena escena;
	Assimp::Importer importer;

	//Normalizar el tamaño completo de la escena y eliminar primitivas que no se puedan convertir a triángulos
	importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

	//Parsear la escena, dejando que la librería aplique todas las matrices de transformación entre otras cosas
	//El resultado es una estructura en forma de árbol, pero con las transformaciones solo tendrá profundidad de 1
	const aiScene* scene = importer.ReadFile(file, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_PreTransformVertices);
	if (nullptr == scene) {
		fprintf(stderr, "%s\n", importer.GetErrorString());
		exit(1);
	}

	//Una escena tiene tantos objetos como mayas, y cada maya puede instanciarse una o más veces
	printf("Assimp: Loaded model %s\n\tAnimations: %u\n\tCameras: %u\n\tLights: %u\n\tMaterials: %u\n\tMeshes: %u\n\tSkeletons: %u\n\tTextures: %u\n",
		scene->mName.C_Str(), scene->mNumAnimations, scene->mNumCameras,
		scene->mNumLights, scene->mNumMaterials, scene->mNumMeshes,
		scene->mNumSkeletons, scene->mNumTextures);

	//Por cada maya, creamos un objeto y su material
	escena.nObjetos = scene->mNumMeshes;
	escena.objs = new objeto[escena.nObjetos];
	escena.buffers = new MeshBuffers[escena.nObjetos];

	//TODO: Arreglar multitextura
	escena.mats = new GLuint[escena.nObjetos];
	for (unsigned int i = 0; i < escena.nObjetos; i++) {
		aiMesh* mesh = scene->mMeshes[i];
		escena.objs[i] = build_objeto(mesh, &escena.buffers[i]);

		//Arreglar multitextura
		//aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
		//escena.mats[i] = build_material(mat);
	}

	//Contamos el número de instancias totales del sistema
	unsigned int nChildren = scene->mRootNode->mNumChildren;
	escena.nInstancias = 0;
	if (!nChildren)
		escena.nInstancias = scene->mRootNode->mNumMeshes;
	for (unsigned int i = 0; i < nChildren; i++)
		escena.nInstancias += scene->mRootNode->mChildren[i]->mNumMeshes;

	//Cada instancia es una referencia a una de las mayas
	escena.instIdx = new unsigned int[escena.nInstancias];
	if (!nChildren) {
		aiNode* node = scene->mRootNode;
		for (unsigned int k = 0; k < node->mNumMeshes; k++)
			escena.instIdx[k] = node->mMeshes[k];
	}
	for (unsigned int i = 0, j = 0; i < nChildren; i++) {
		aiNode* node = scene->mRootNode->mChildren[i];
		for (unsigned int k = 0; k < node->mNumMeshes; k++)
			escena.instIdx[j++] = node->mMeshes[k];
	}

	return escena;
}

void limpiar_escena(struct escena* escena) {
	if (!escena) {
		return;
	}

	// Limpiamos los VAOs, VBOs y texturas de cada objeto
	if (escena->objs && escena->buffers) {
		for (unsigned int i = 0; i < escena->nObjetos; i++) {
			if (escena->objs[i].VAO != 0) {
				glDeleteVertexArrays(1, &escena->objs[i].VAO);
				escena->objs[i].VAO = 0;
			}

			MeshBuffers& buffers = escena->buffers[i];
			if (buffers.pos != 0) {
				glDeleteBuffers(1, &buffers.pos);
				buffers.pos = 0;
			}
			if (buffers.norm != 0) {
				glDeleteBuffers(1, &buffers.norm);
				buffers.norm = 0;
			}
			if (buffers.uv != 0) {
				glDeleteBuffers(1, &buffers.uv);
				buffers.uv = 0;
			}
			if (buffers.idx != 0) {
				glDeleteBuffers(1, &buffers.idx);
				buffers.idx = 0;
			}
		}
	}

	if (escena->mats) {
		for (unsigned int i = 0; i < escena->nObjetos; i++) {
			if (escena->mats[i] != 0) {
				glDeleteTextures(1, &escena->mats[i]);
				escena->mats[i] = 0;
			}
		}
	}

	delete []escena->objs;
	delete []escena->mats;
	delete []escena->instIdx;
	delete []escena->buffers;

	escena->objs = nullptr;
	escena->mats = nullptr;
	escena->instIdx = nullptr;
	escena->buffers = nullptr;
	escena->nObjetos = 0;
	escena->nInstancias = 0;
}
