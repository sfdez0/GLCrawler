#include "pathfinding.h"

/**
 * Constructor de PathfindingModule
 * @param side_size Número de filas y columnas del mapa (cuadrado)
 * @param cellSize Tamaño de cada celda en unidades 3D (por defecto 2.0f)
 */
PathfindingModule::PathfindingModule(int side_size, float cellSize_)
    : mazeWidth(side_size), mazeHeight(side_size), cellSize(cellSize_), mazeCenterXZ((side_size * cellSize_) / 2.0f) {
    // Configuramos astar con nuestros datos
    generator.setWorldSize({side_size, side_size});
    generator.setDiagonalMovement(false);
    generator.setHeuristic(AStar::Heuristic::manhattan);
}

/**
 * Función para establecer los datos del mapa
 * @param map Array de datos del mapa (0 = vacio, 1 = muro)
 */
void PathfindingModule::setMazeData(const int map[]) {
    // Convertimos el array 1D en vector 2D (input de astar)
	std::vector<std::vector<int>> data(mazeWidth, std::vector<int>(mazeWidth));
	for (int i = 0; i < mazeWidth; i++) {
		for (int j = 0; j < mazeWidth; j++) {
			data[i][j] = map[i * mazeWidth + j];
		}
	}

    mazeData = data;
    
    // Limpiamos colisiones de astar
    generator.clearCollisions();

    // Añadimos los muros como colisiones para astar
    for (int y = 0; y < mazeWidth; ++y) {
        for (int x = 0; x < mazeWidth; ++x) {
            if (y < data.size() && x < data[y].size() && data[y][x] != 0) {
                generator.addCollision({x, y});
            }
        }
    }
}

/**
 * Funcion para encontrar el camino entre dos puntos del laberinto
 * @param startX Coordenada X inicial
 * @param startY Coordenada Y inicial  
 * @param targetX Coordenada X destino
 * @param targetY Coordenada Y destino
 * @return Vector de puntos 3D que representan el camino
 */
std::vector<vec3> PathfindingModule::findPath(float startX, float startY, float targetX, float targetY) {
    // Convertimos coordenadas del mundo a coordenadas del mapa
    AStar::Vec2i startMap = worldToMapCoordinates(startX, startY);
    AStar::Vec2i targetMap = worldToMapCoordinates(targetX, targetY);

    // Buscamos con astar el camino en coordenadas del mapa
    auto path2D = generator.findPath({startMap.x, startMap.y}, {targetMap.x, targetMap.y});

    // Convertimos el camino a coordenadas del mundo 3D
    std::vector<vec3> path3D;
    for (const auto& coord : path2D) {
        path3D.push_back(mapToWorldCoordinates(coord.x, coord.y));
    }

    // Invertimos el camino (astar devuelve de destino a origen)
    std::reverse(path3D.begin(), path3D.end());

    // Borramos el primer punto (posicion actual)
    if (path3D.size() > 1) {
        path3D.erase(path3D.begin());
    }

    return path3D;
}

/**
 * Funcion para convertir coordenadas del juego a coordenadas del mapa
 * @param x Coordenada X en el mundo
 * @param z Coordenada Z en el mundo
 * @return Coordenadas en el mapa 2D
 */
AStar::Vec2i PathfindingModule::worldToMapCoordinates(float x, float z) {
    // Redondeamos a la celda más cercana
    int mapX = std::lround((x + mazeCenterXZ) / cellSize);
    int mapY = std::lround((z + mazeCenterXZ) / cellSize);


    // Forzamos limite entre 0 y tamaño del mapa
    mapX = std::clamp(mapX, 0, mazeWidth - 1);
    mapY = std::clamp(mapY, 0, mazeHeight - 1);

    return {mapX, mapY};
}

/**
 * Funcion para convertir coordenadas del mapa a coordenadas del juego
 * @param mapX Coordenada X en el mapa
 * @param mapY Coordenada Y en el mapa
 * @return Coordenadas en el mundo 3D
 */
vec3 PathfindingModule::mapToWorldCoordinates(int mapX, int mapY) {
    float worldX = mapX * cellSize - mazeCenterXZ;
    float worldZ = mapY * cellSize - mazeCenterXZ;
    float worldY = 0.0f; // Altura del suelo

    return vec3(worldX, worldY, worldZ);
}
