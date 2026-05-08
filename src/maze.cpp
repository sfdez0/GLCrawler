#include "maze.h"

/**
 * Constructor de Maze
 * `r` Número de filas y columnas del mapa (cuadrado)
 * `size` Tamaño de cada celda en unidades 3D (por defecto 2.0f)
 */
Maze::Maze(int side_size, float size) 
: rows(side_size), columns(side_size), tile_size(size) {
    // Inicializamos la matriz con cero
    map.resize(rows, std::vector<int>(columns, 0));
}

/**
 * Destructor de Maze
 */
Maze::~Maze() {
    // Limpiamos vectores
    map.clear();
}

/**
 * Función para establecer el mapa a partir de un array de enteros
 * `datos` Arreglo 1D con los datos del mapa (1 = muro, 0 = vacío)
 * `r` Número de filas del mapa (debe coincidir con el número de columnas para un mapa cuadrado)
 */
void Maze::setMap(const int* datos, int r) {
    // Verificamos si el tamaño del mapa ha cambiado
    if (r != rows || r != columns) {
        // Limpiamos y redimensionamos la matriz
        rows = r;
        columns = r;
        map.clear();
        map.resize(rows, std::vector<int>(columns));
    }
    
    // Copiamos los datos a la matriz
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            map[i][j] = datos[i * columns + j];
        }
    }
    
    // Generamos las bounding boxes para todos los muros
    wallBoundingBoxes.clear();
    float maze_center_xz = (rows * tile_size) / 2.0f;
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            if (map[i][j] == 1) {
                // Calculamos las coordenadas en el espacio 3D
                float posX = j * tile_size - maze_center_xz;
                float posZ = i * tile_size - maze_center_xz;
                
                // Creamos la bounding box del muro (eje Y no es necesario)
                BoundingBox bbox;
                bbox.min = vec3(posX - tile_size / 2.0f, 0.0f, posZ - tile_size / 2.0f);
                bbox.max = vec3(posX + tile_size / 2.0f, 0.0f, posZ + tile_size / 2.0f);
                
                wallBoundingBoxes.push_back(bbox);
            }
        }
    }
}

/**
 * Función para establecer el valor de una celda específica
 * `r` Índice de fila
 * `c` Índice de columna
 * `value` Valor a establecer (1 = muro, 0 = vacío)
 */
void Maze::setGrid(int r, int c, int value) {
    if (r >= 0 && r < rows && c >= 0 && c < columns) {
        map[r][c] = value;
    }
}

/**
 * Función para obtener el valor de una celda específica
 * `r` Índice de fila
 * `c` Índice de columna
 */
int Maze::getGrid(int r, int c) {
    if (r >= 0 && r < rows && c >= 0 && c < columns) {
        return map[r][c];
    }
    return -1;
}

/**
 * Función para obtener el número de filas
 */
int Maze::getRows() {
    return rows;
}

/**
 * Función para obtener el número de columnas
 */
int Maze::getColumns() {
    return columns;
}

/**
 * Función para obtener el tamaño de cada celda
 */
float Maze::getTileSize() {
    return tile_size;
}

/**
 * Función para obtener las posiciones 3D de los muros en el mapa
 */
std::vector<vec3> Maze::getWallPositions() {
    std::vector<vec3> positions;

    // Iteramos por la matriz para encontrar los muros
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
            // Buscamos los muros (1)
            if (map[i][j] == 1) {
                // Convertimos la posición de matriz a coordenadas 3D (división entre 2 para centrar en origen)
                float x = j * tile_size - (columns * tile_size) / 2.0f;
                float z = i * tile_size - (rows * tile_size) / 2.0f;
                positions.push_back(vec3(x, 0.0f, z));
            }
        }
    }
    
    return positions;
}

/**
 * Función para verificar colisión AABB entre la cámara (como caja) y los muros
 * `position` Posición central de la cámara
 * `radius` Radio/mitad del ancho de la caja de colisión de la cámara
 * Devuelve true si hay colisión, false si está libre
 */
bool Maze::checkCollisionWithBoundingBoxes(vec3 position, float radius) {
    // AABB de la cámara (caja rectangular centrada en position)
    float cam_min_x = position.x - radius;
    float cam_max_x = position.x + radius;
    float cam_min_z = position.z - radius;
    float cam_max_z = position.z + radius;
    
    // Verificamos colisión con todas las bounding boxes de los muros
    for (const BoundingBox& wall : wallBoundingBoxes) {
        // Comprobamos si las dos AABBs se solapan en XZ
        if (!(cam_max_x < wall.min.x || cam_min_x > wall.max.x ||
              cam_max_z < wall.min.z || cam_min_z > wall.max.z)) {
            return true; // Hay colisión
        }
    }
    
    return false; // No hay colisión
}
