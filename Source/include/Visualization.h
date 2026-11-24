#pragma once

#include <string>
#include <vector>

#include "Maze.h"
#include "Graph.h"
#include "Dijkstra.h"

namespace visualization {

    // What a host (console, D's window, E's tests) paints for a single cell.
    // The core module only decides the STATE; turning it into a colour or a
    // character is the host's job (see the GUI in Source/app_gui).
    enum class CellState {
        Wall,     // '1'
        Floor,    // walkable, plain cost 1
        Weighted, // walkable, cost 2..9 (the host draws the digit)
        Visited,  // finalised by Dijkstra within the current frame
        Path,     // on the shortest path
        Start,    // 'S'
        Goal      // 'E'
    };

    // One cell of one frame.
    struct Cell {
        CellState state = CellState::Floor;
        int weight = 1;        // meaningful when state == Weighted (2..9)
    };

    // The numbers the assignment asks D to display.
    struct SearchStats {
        bool reachable = false;
        long long totalCost = 0;   // shortest path cost (meaningless if !reachable)
        int moves = 0;             // number of steps = path.size() - 1, 0 if unreachable
        int nodesVisitedShown = 0; // how many visited nodes are visible in this frame
        int nodesVisitedTotal = 0; // total finalised nodes available to animate
        double searchMs = 0.0;     // wall-clock time of the Dijkstra search
    };

    // A fully host-agnostic description of one frame of the search.
    // A renderer turns this into pixels (GUI) or text (console).
    struct Frame {
        int rows = 0;
        int cols = 0;
        int startRow = -1, startCol = -1;
        int endRow = -1, endCol = -1;
        SearchStats stats;
        std::vector<Cell> cells;   // row-major, size == rows * cols
        int visitedUpTo = -1;      // animation cursor actually used to build this frame
    };

    class Renderer {
    public:
        // Build one frame of the search.
        //
        //   result      - output of dijkstra::findShortestPath. May be default-
        //                 constructed (the "maze only" frame, before any search).
        //   searchMs    - measured Dijkstra wall time, shown in the stats panel.
        //   visitedUpTo - animation cursor, in the range 0..result.nodesExpanded.
        //                 Only the first `visitedUpTo` finalised nodes are drawn
        //                 as Visited. Pass -1 to show every visited node at once.
        //
        // Cell priority (highest first): Start > Goal > Wall > Path > Visited
        // > Weighted > Floor. Start/Goal/Path always win over the visited shading.
        static Frame buildFrame(const Maze& maze,
                                const graph::Graph& g,
                                const dijkstra::Result& result,
                                double searchMs,
                                int visitedUpTo = -1);
    };
}  // namespace visualization
