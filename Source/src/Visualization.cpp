// Source/src/Visualization.cpp
// Person D - Visualization.
//
// The whole module lives here: it turns A's maze + B's graph + C's Dijkstra
// result into host-agnostic "frames" that any UI (the GUI window, a console,
// E's tests) can paint. See docs/Q&A.md: "the same code works for a console,
// D's window and E's tests."

#include "Visualization.h"

using dijkstra::Result;
using graph::Graph;

namespace visualization {

namespace {

    inline bool g_isValid(int id, int rows, int cols) {
        return id >= 0 && id < rows * cols;
    }

    // Draws the visited shading onto `frame.cells` from the Dijkstra result.
    // Only the first `shown` finalised nodes are marked; Start/Goal/Path cells
    // keep their state (they win over the visited shading).
    void applyVisited(const Result& result, Frame& frame, int shown) {
        const int total = static_cast<int>(result.nodesExpanded);
        if (shown < 0) {
            shown = 0;
        }
        if (shown > total) {
            shown = total;
        }

        for (int i = 0; i < shown; ++i) {
            const int id = result.visitOrder[static_cast<std::size_t>(i)];
            if (!g_isValid(id, frame.rows, frame.cols)) {
                continue;
            }
            Cell& cell = frame.cells[static_cast<std::size_t>(id)];
            // Priority: Start > Goal > Path > Visited.
            if (cell.state == CellState::Start || cell.state == CellState::Goal ||
                cell.state == CellState::Path) {
                continue;
            }
            cell.state = CellState::Visited;
        }

        frame.stats.nodesVisitedShown = shown;
        frame.stats.nodesVisitedTotal = total;
    }

}  // namespace

Frame Renderer::buildFrame(const Maze& maze,
                           const Graph& g,
                           const Result& result,
                           double searchMs,
                           int visitedUpTo) {
    Frame frame;
    frame.rows = maze.rows();
    frame.cols = maze.cols();
    frame.cells.assign(static_cast<std::size_t>(frame.rows) * frame.cols, Cell{});
    frame.visitedUpTo = visitedUpTo;

    // Base layer: every cell starts as wall or floor, per A's grid.
    for (int r = 0; r < frame.rows; ++r) {
        for (int c = 0; c < frame.cols; ++c) {
            const char ch = maze.at(r, c);
            Cell& cell = frame.cells[static_cast<std::size_t>(c + r * frame.cols)];
            if (ch == Maze::WALL) {
                cell.state = CellState::Wall;
            } else if (ch >= '2' && ch <= '9') {
                cell.state = CellState::Weighted;
                cell.weight = ch - '0';
            } else {
                cell.state = CellState::Floor;
            }
        }
    }

    // Start / Goal markers, straight from A's maze.
    const Position start = maze.start();
    const Position goal = maze.end();
    if (maze.isWalkable(start.row, start.col)) {
        frame.startRow = start.row;
        frame.startCol = start.col;
        frame.cells[static_cast<std::size_t>(start.col + start.row * frame.cols)].state =
            CellState::Start;
    }
    if (maze.isWalkable(goal.row, goal.col)) {
        frame.endRow = goal.row;
        frame.endCol = goal.col;
        frame.cells[static_cast<std::size_t>(goal.col + goal.row * frame.cols)].state =
            CellState::Goal;
    }

    // The shortest path, mapped back to cells with B's id <-> position helper.
    // Priority: Start/Goal keep their markers even though the path runs
    // through them; only the interior cells are painted as Path.
    for (int id : result.path) {
        const Position p = g.toPosition(id);
        if (!maze.isWalkable(p.row, p.col)) {
            continue;  // cannot happen for a graph built from this maze
        }
        Cell& cell = frame.cells[static_cast<std::size_t>(p.col + p.row * frame.cols)];
        if (cell.state == CellState::Start || cell.state == CellState::Goal) {
            continue;
        }
        cell.state = CellState::Path;
    }

    // The visited shading (fills stats.nodesVisitedShown / nodesVisitedTotal).
    const int shown = (visitedUpTo < 0) ? static_cast<int>(result.nodesExpanded) : visitedUpTo;
    applyVisited(result, frame, shown);

    // The rest of the stats panel. Do NOT re-default frame.stats here: the
    // visited counts were just computed above and would be wiped.
    frame.stats.reachable = result.reachable;
    frame.stats.searchMs = searchMs;
    if (result.reachable) {
        frame.stats.totalCost = result.totalCost;
        frame.stats.moves = static_cast<int>(result.path.size()) - 1;
    }

    return frame;
}

}  // namespace visualization
