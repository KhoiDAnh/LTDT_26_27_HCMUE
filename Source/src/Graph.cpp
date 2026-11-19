#include "Graph.h"
#include <cmath>

namespace graph {

    Graph::Graph()
        : gridRows(0),
          gridCols(0),
          totalVertices(0),
          totalEdges(0),
          startId(-1),
          endId(-1) {}

    Graph::Graph(int numVertices)
        : gridRows(0),
          gridCols(0),
          totalVertices(0),
          totalEdges(0),
          startId(-1),
          endId(-1) {
        reset(numVertices);
    }

    Graph::Graph(int rows, int cols)
        : gridRows(0),
          gridCols(0),
          totalVertices(0),
          totalEdges(0),
          startId(-1),
          endId(-1) {
        reset(rows, cols);
    }

    void Graph::reset(int numVertices) {
        gridRows = 0;
        gridCols = 0;
        totalVertices = numVertices > 0 ? numVertices : 0;
        totalEdges = 0;
        startId = -1;
        endId = -1;
        adjList.assign(totalVertices, std::vector<Edge>());
        walkableMask.assign(totalVertices, true);
    }

    void Graph::reset(int rows, int cols) {
        if (rows <= 0 || cols <= 0) {
            reset(0);
            return;
        }
        gridRows = rows;
        gridCols = cols;
        totalVertices = rows * cols;
        totalEdges = 0;
        startId = -1;
        endId = -1;
        adjList.assign(totalVertices, std::vector<Edge>());
        walkableMask.assign(totalVertices, false);
    }

    void Graph::addEdge(int u, int v, int weight) {
        if (!isValidVertex(u) || !isValidVertex(v) || weight < 0) {
            return;
        }
        adjList[u].push_back(Edge{v, weight});
        totalEdges++;
    }

    void Graph::addUndirectedEdge(int u, int v, int weight) {
        addEdge(u, v, weight);
        addEdge(v, u, weight);
    }

    void Graph::buildFromMaze(const Maze& maze, bool diagonal) {
        int rCount = maze.rows();
        int cCount = maze.cols();
        reset(rCount, cCount);

        if (rCount <= 0 || cCount <= 0) {
            return;
        }

        Position startPos = maze.start();
        Position endPos = maze.end();
        if (maze.isWalkable(startPos.row, startPos.col)) {
            startId = toVertexId(startPos);
        }
        if (maze.isWalkable(endPos.row, endPos.col)) {
            endId = toVertexId(endPos);
        }

        for (int r = 0; r < rCount; ++r) {
            for (int c = 0; c < cCount; ++c) {
                if (!maze.isWalkable(r, c)) {
                    continue;
                }

                int u = toVertexId(r, c);
                walkableMask[u] = true;
                Position current{r, c};
                std::vector<Position> nextCells = maze.neighbors(current, diagonal);

                for (const Position& next : nextCells) {
                    int v = toVertexId(next.row, next.col);
                    if (!diagonal) {
                        int stepWeight = maze.cost(next.row, next.col);
                        if (stepWeight > 0) {
                            addEdge(u, v, stepWeight);
                        }
                    } else {
                        int stepWeight = static_cast<int>(std::llround(maze.moveCost(current, next)));
                        if (stepWeight > 0) {
                            addEdge(u, v, stepWeight);
                        }
                    }
                }
            }
        }
    }

    Graph Graph::fromMaze(const Maze& maze, bool diagonal) {
        Graph g;
        g.buildFromMaze(maze, diagonal);
        return g;
    }

    int Graph::toVertexId(int row, int col) const {
        if (gridCols <= 0 || row < 0 || row >= gridRows || col < 0 || col >= gridCols) {
            return -1;
        }
        return row * gridCols + col;
    }

    int Graph::toVertexId(const Position& pos) const {
        return toVertexId(pos.row, pos.col);
    }

    Position Graph::toPosition(int vertexId) const {
        if (gridCols <= 0 || !isValidVertex(vertexId)) {
            return Position{-1, -1};
        }
        return Position{vertexId / gridCols, vertexId % gridCols};
    }

    std::pair<int, int> Graph::toCoord(int vertexId) const {
        Position pos = toPosition(vertexId);
        return {pos.row, pos.col};
    }

    bool Graph::isValidVertex(int vertexId) const {
        return vertexId >= 0 && vertexId < totalVertices;
    }

    bool Graph::isWalkableVertex(int vertexId) const {
        return isValidVertex(vertexId) && walkableMask[vertexId];
    }

    int Graph::numVertices() const {
        return totalVertices;
    }

    int Graph::vertexCount() const {
        return totalVertices;
    }

    int Graph::size() const {
        return totalVertices;
    }

    int Graph::numEdges() const {
        return totalEdges;
    }

    int Graph::edgeCount() const {
        return totalEdges;
    }

    int Graph::rows() const {
        return gridRows;
    }

    int Graph::cols() const {
        return gridCols;
    }

    int Graph::startVertex() const {
        return startId;
    }

    int Graph::endVertex() const {
        return endId;
    }

    const std::vector<Edge>& Graph::neighbors(int u) const {
        static const std::vector<Edge> emptyList;
        if (!isValidVertex(u)) {
            return emptyList;
        }
        return adjList[u];
    }

    const std::vector<Edge>& Graph::getNeighbors(int u) const {
        return neighbors(u);
    }

    const std::vector<std::vector<Edge>>& Graph::adjacencyList() const {
        return adjList;
    }
} // namespace graph
