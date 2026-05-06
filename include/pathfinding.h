#pragma once

#include <algorithm>
#include "GpO.h"
#include "AStar.hpp"

/**
 * Modulo de pathfinding para el laberinto
 * Hace de interfaz entre el sistema de coordenadas del juego y la libreria astar
 */
class PathfindingModule {
private:
    AStar::Generator generator;
    int mazeWidth, mazeHeight;
    float cellSize;
    float mazeCenterXZ;
    std::vector<std::vector<int>> mazeData;

public:
    PathfindingModule(int side_size, float cellSize = 4.0f);
    ~PathfindingModule();

    void setMazeData(const int data[]);

    std::vector<vec3> findPath(float startX, float startY, float targetX, float targetY);

    void setDiagonalMovement(bool enable);

    void setHeuristic(AStar::HeuristicFunction heuristic);

    AStar::Vec2i worldToMapCoordinates(float x, float z);

    vec3 mapToWorldCoordinates(int mapX, int mapY);
};
