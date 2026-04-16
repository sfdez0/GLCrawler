#include <stdio.h>
#include <maze.h>

/**
 * Constructor de Maze
 * `r` Número de filas y columnas del mapa (cuadrado)
 * `size` Tamaño de cada celda en unidades 3D (por defecto 2.0f)
 */
Maze::Maze(int r, float size) 
: rows(r), columns(r), tile_size(size) {
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
 * Función para verificar si una posición 3D es transitable (no es un muro)
 * `x` Coordenada X en el espacio 3D
 * `z` Coordenada Z en el espacio 3D
 * Devuelve true si la posición es transitable, false si es un muro o fuera de límites
 */
bool Maze::isWalkable(vec3 position) {
    float x = position.x;
    float z = position.z;
    float y = position.y;

    // Convertimos las coordenadas 3D a índices de matriz
    int col = (int)((x + (columns * tile_size) / 2.0f) / tile_size);
    int row = (int)((z + (rows * tile_size) / 2.0f) / tile_size);
    
    // Verificamos si los indices están dentro de los límites del mapa
    if (row >= 0 && row < rows && col >= 0 && col < columns) {
        // Es transitable si el valor es 0 (vacío)
        return map[row][col] == 0;
    }
    return false;
}
