/*
* Step	Plain meaning
    1	Scan the grid and collect every walkable cell
    2	Fewer than 2 walkable cells means fail with a message
    3	Make a seeded random generator (same family as A's, mt19937)
    4	Pick two different cells from the list
    5	Move S and E there with A's setStart and setEnd
    6	Ask A's isSolvable() whether a route exists
    7	If yes, done. If not, try another pair, up to a fixed number of attempts, then restore the original and fail.
 */

#include "Scenario.h"

#include <random>
#include <string>
#include <vector>

namespace scenario {

    namespace {

        // How many random pairs we try before giving up.
        constexpr int kMaxAttempts = 100;

        // Upper limit for the maze size, to catch typos like 100000 that would exhaust memory.
        // Raise it if E's benchmarks need bigger mazes.
        constexpr int kMaxSize = 2000;

        void setErr(std::string* err, const std::string& msg) {
            if (err) {
                *err = msg;
            }
        }

        // Applies a new Start/End pair to `maze`. Returns false if A's setters refuse.
        // setStart() refuses the current End cell and setEnd() refuses the current Start cell,
        // so when the new Start sits on the old End, the End must move first.
        bool applyPair(Maze& maze, const Position& s, const Position& e) {
            if (s == maze.end()) {
                if (e == maze.start()) {
                    return false;  // a pure swap: each setter would block the other
                }
                return maze.setEnd(e.row, e.col) && maze.setStart(s.row, s.col);
            }
            return maze.setStart(s.row, s.col) && maze.setEnd(e.row, e.col);
        }

    }  // namespace

    bool randomizeStartEnd(Maze& maze, unsigned seed, std::string* err) {
        // 1. Collect every walkable cell (floor, weighted floor, and the current S and E).
        std::vector<Position> cells;
        for (int r = 0; r < maze.rows(); ++r) {
            for (int c = 0; c < maze.cols(); ++c) {
                if (maze.isWalkable(r, c)) {
                    cells.push_back(Position{r, c});
                }
            }
        }

        // 2. We need two different cells.
        if (cells.size() < 2) {
            setErr(err, "Need at least 2 walkable cells to place Start and End");
            return false;
        }
        const int count = static_cast<int>(cells.size());

        // 3. Seeded generator: same seed, same sequence of picks.
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> pickFirst(0, count - 1);
        std::uniform_int_distribution<int> pickSecond(0, count - 2);

        for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
            // 4. Two different cells: the second is drawn from the other count-1 cells,
            //    skipping over index i.
            const int i = pickFirst(rng);
            int j = pickSecond(rng);
            if (j >= i) {
                ++j;
            }

            // 5. Try on a copy, so the real maze stays untouched unless we succeed.
            Maze trial = maze;
            if (!applyPair(trial, cells[i], cells[j])) {
                continue;
            }

            // 6. Is there a route from the new Start to the new End? (4 directions,
            //    the same as the graph we build.)
            if (trial.isSolvable()) {
                maze = trial;  // 7. success: commit the change
                return true;
            }
        }

        setErr(err, "No reachable Start/End pair found after " +
                        std::to_string(kMaxAttempts) + " attempts");
        return false;
    }

    bool makeScenario(const Settings& settings, Maze& out, std::string* err) {
        if (settings.size < 5 || settings.size > kMaxSize) {
            setErr(err, "Size must be between 5 and " + std::to_string(kMaxSize));
            return false;
        }

        // Build on a local maze, so `out` is untouched if anything fails.
        Maze maze;

        // 1. Generate. Each stage gets its own seed (seed, seed+1, seed+2),
        //    so the stages do not share one random stream.
        if (settings.kind == MazeKind::Perfect) {
            maze.generateRandom(settings.size, settings.size, settings.seed);
        } else {
            maze.generateByWallRatio(settings.size, settings.size, settings.wallRatio,
                                     settings.seed);
        }

        // 2. Terrain weights (skipped when the ratio is 0).
        if (settings.weightRatio > 0.0) {
            maze.addWeights(settings.weightRatio, settings.maxWeight, settings.seed + 1);
        }

        // 3. Random Start/End (F1).
        if (settings.randomStartEnd &&
            !randomizeStartEnd(maze, settings.seed + 2, err)) {
            return false;
            }

        out = maze;  // commit
        return true;
    }
}  // namespace scenario