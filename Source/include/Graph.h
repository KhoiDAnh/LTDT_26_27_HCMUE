#pragma once

#include <utility>
#include <vector>
#include "Maze.h"

namespace graph {

    struct Edge {
        int to;
        int weight;
    };

    class Graph {
    private:
        int gridRows;
        int gridCols;
        int totalVertices;
        int totalEdges;
        int startId;
        int endId;
        std::vector<std::vector<Edge>> adjList;
        std::vector<bool> walkableMask;

    public:
        Graph();
        explicit Graph(int numVertices);
        Graph(int rows, int cols);

        void reset(int numVertices);
        void reset(int rows, int cols);

        void addEdge(int u, int v, int weight = 1);
        void addUndirectedEdge(int u, int v, int weight = 1);

        void buildFromMaze(const Maze& maze, bool diagonal = false);
        static Graph fromMaze(const Maze& maze, bool diagonal = false);

        int toVertexId(int row, int col) const;
        int toVertexId(const Position& pos) const;
        Position toPosition(int vertexId) const;
        std::pair<int, int> toCoord(int vertexId) const;

        bool isValidVertex(int vertexId) const;
        bool isWalkableVertex(int vertexId) const;

        int numVertices() const;
        int vertexCount() const;
        int size() const;
        int numEdges() const;
        int edgeCount() const;
        int rows() const;
        int cols() const;
        int startVertex() const;
        int endVertex() const;

        const std::vector<Edge>& neighbors(int u) const;
        const std::vector<Edge>& getNeighbors(int u) const;
        const std::vector<std::vector<Edge>>& adjacencyList() const;
    };
} // namespace graph