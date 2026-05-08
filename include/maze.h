#pragma once

#include <GpO.h>

// Estructura para representar una Bounding Box
struct BoundingBox {
    vec3 min;
    vec3 max;
};

// Estructura para representar el mapa
class Maze {
private:
    std::vector<std::vector<int>> map; // Matriz 2D (1 = muro, 0 = vacio)
    int rows;                          // Número de filas del mapa
    int columns;                       // Número de columnas del mapa
    float tile_size;                   // Tamaño de cada celda en unidades 3D
    std::vector<BoundingBox> wallBoundingBoxes; // Vector con bounding boxes de todos los muros

public:
    Maze(int side_size, float cellSize = 2.0f);
    ~Maze();

    void setMap(const int *data, int r);
    void setGrid(int row, int col, int value);

    int getGrid(int row, int col);
    int getRows();
    int getColumns();
    float getTileSize();
    std::vector<vec3> getWallPositions();

    bool checkCollisionWithBoundingBoxes(vec3 position, float radius);
};
