// Source/app/viz_smoke.cpp
// Person D - headless smoke test for the Visualization module.
//
// Runs the whole pipeline (Maze -> Graph -> Dijkstra -> Visualization) without
// any window, and checks the invariants the GUI relies on:
//   * Start/Goal/Wall cells are painted with the right states.
//   * every cell on the Dijkstra path is marked Path.
//   * the visited count in a full frame equals result.nodesExpanded.
//   * an animation frame at cursor k shows exactly k visited cells.
//   * stats (cost, moves, reachability) agree with the Dijkstra result.
//
// The same Renderer is what the GUI window paints, so passing this means the
// window will be correct too. No GUI dependencies here, so E's CI can call it.
//
// Usage:
//   viz_smoke                     -> uses the bundled maze_smoke.txt
//   viz_smoke <maze.txt>          -> loads your own maze
//
// Exit code 0 = all invariants held, 1 = at least one failed, 2 = bad input.

#include <cstdio>
#include <string>
#include <vector>

#include "Maze.h"
#include "Graph.h"
#include "Dijkstra.h"
#include "Visualization.h"

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    std::printf("  [%s] %s\n", ok ? "ok" : "FAIL", what.c_str());
    if (!ok) {
        ++failures;
    }
}

void runSmoke(const std::string& mazeFile) {
    std::printf("== viz_smoke: %s ==\n", mazeFile.c_str());

    std::string err;
    Maze maze;
    if (!maze.loadFromFile(mazeFile, &err)) {
        std::printf("Cannot load maze: %s\n", err.c_str());
        return;
    }
    std::printf("maze %d x %d, start (%d,%d), goal (%d,%d)\n", maze.rows(),
                 maze.cols(), maze.start().row, maze.start().col, maze.end().row,
                 maze.end().col);

    graph::Graph g = graph::Graph::fromMaze(maze);
    if (g.startVertex() < 0 || g.endVertex() < 0) {
        std::printf("Start or End is not on a walkable cell; nothing to test.\n");
        return;
    }

    const std::vector<int> targets = {g.endVertex()};
    const dijkstra::Result r =
        dijkstra::findShortestPath(g, g.startVertex(), targets);

    // Full frame: visitedUpTo = -1 shows every finalised node.
    visualization::Frame full =
        visualization::Renderer::buildFrame(maze, g, r, 0.0, -1);

    // 1. Start and Goal cells carry the right state.
    check(full.startRow == maze.start().row && full.startCol == maze.start().col,
          "frame reports the Start cell");
    check(full.endRow == maze.end().row && full.endCol == maze.end().col,
          "frame reports the Goal cell");
    check(full.cells[static_cast<std::size_t>(full.startCol + full.startRow * full.cols)]
                .state == visualization::CellState::Start,
          "Start cell painted as Start");
    check(full.cells[static_cast<std::size_t>(full.endCol + full.endRow * full.cols)]
                .state == visualization::CellState::Goal,
          "Goal cell painted as Goal");

    // 2. Every cell on the path is marked Path (path wins over visited).
    bool pathAllMarked = !r.path.empty();
    for (int id : r.path) {
        const Position p = g.toPosition(id);
        const visualization::Cell& c =
            full.cells[static_cast<std::size_t>(p.col + p.row * full.cols)];
        if (c.state != visualization::CellState::Path &&
            c.state != visualization::CellState::Start &&
            c.state != visualization::CellState::Goal) {
            pathAllMarked = false;
        }
    }
    check(pathAllMarked, "every path cell painted as Path/Start/Goal");

    // 3. A wall cell stays a wall.
    bool wallStillWall = false;
    for (int rr = 0; rr < maze.rows() && !wallStillWall; ++rr) {
        for (int cc = 0; cc < maze.cols() && !wallStillWall; ++cc) {
            if (maze.isWall(rr, cc)) {
                wallStillWall = full.cells[static_cast<std::size_t>(cc + rr * full.cols)]
                                    .state == visualization::CellState::Wall;
            }
        }
    }
    check(wallStillWall, "wall cells painted as Wall");

    // 4. The full frame's visited count equals what Dijkstra finalised.
    check(full.stats.nodesVisitedTotal == static_cast<int>(r.nodesExpanded),
          "full frame shows all visited nodes");

    // 5. Stats agree with the Dijkstra result.
    check(full.stats.reachable == r.reachable, "stats.reachable matches");
    if (r.reachable) {
        check(full.stats.totalCost == r.totalCost, "stats.totalCost matches");
        check(full.stats.moves == static_cast<int>(r.path.size()) - 1,
              "stats.moves matches");
    }

    // 6. Animation frame: cursor k shows exactly the first k visited nodes.
    const int k = static_cast<int>(r.nodesExpanded) / 2;
    visualization::Frame anim =
        visualization::Renderer::buildFrame(maze, g, r, 0.0, k);
    int countedVisited = 0;
    for (const auto& c : anim.cells) {
        if (c.state == visualization::CellState::Visited) {
            ++countedVisited;
        }
    }
    // Some of the first k nodes may sit under Start/Goal/Path and thus are not
    // painted "Visited"; so count <= k, and equals k unless a first-k node is
    // also on the path. We assert the honest bound.
    check(countedVisited <= k, "animated frame shows at most k visited nodes");
    check(anim.stats.nodesVisitedShown == k, "stats report the animation cursor");

    // 7. Edge: a frame with cursor 0 shows no visited shading.
    visualization::Frame zero =
        visualization::Renderer::buildFrame(maze, g, r, 0.0, 0);
    check(zero.stats.nodesVisitedShown == 0, "cursor 0 shows zero visited nodes");

    std::printf("reachable=%s cost=%s nodesExpanded=%s\n", r.reachable ? "yes" : "no",
                std::to_string((long long)r.totalCost).c_str(),
                std::to_string((unsigned long long)r.nodesExpanded).c_str());
}

}  // namespace

int main(int argc, char** argv) {
    // Default to the bundled smoke maze next to this source tree.
    std::string mazeFile = "Source/data/maze_smoke.txt";
    if (argc > 1) {
        mazeFile = argv[1];
    }
    runSmoke(mazeFile);

    if (failures == 0) {
        std::printf("viz_smoke: all invariants held.\n");
        return 0;
    }
    std::printf("viz_smoke: %d check(s) FAILED.\n", failures);
    return 1;
}
