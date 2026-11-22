// Source/include/Scenario.h
#pragma once

#include <string>

#include "Maze.h"

namespace scenario {

    // Which of A's generators to use.
    enum class MazeKind {
        Perfect,    // A's generateRandom: a perfect maze plus a few extra openings
        WallRatio   // A's generateByWallRatio: random walls at a given density
    };

    // Everything needed to describe one random scenario. All fields have defaults.
    struct Settings {
        int size = 21;                       // the maze is size x size cells
        unsigned seed = 0;                   // same settings + same seed -> same scenario
        MazeKind kind = MazeKind::Perfect;
        double wallRatio = 0.3;              // WallRatio only; A clamps it to 0..0.6
        double weightRatio = 0.0;            // share of floor cells given a cost; 0 = no weights
        int maxWeight = 5;                   // costs are 2..maxWeight; A clamps it to 2..9
        bool randomStartEnd = true;          // false = keep the generator's corner Start/End
    };

    // Moves Start and End to two different random walkable cells such that a route exists.
    // Same maze + same seed -> same result.
    // On success: returns true and `maze` has the new Start/End.
    // On failure: returns false, the reason is written to `err`, and `maze` is left untouched.
    bool randomizeStartEnd(Maze& maze, unsigned seed, std::string* err = nullptr);

    // Builds a complete random scenario from `settings` and stores it in `out`:
    // generate the maze, add weights, then randomise Start/End.
    // Note: A's Perfect generator rounds the size up to an odd number, so check out.rows().
    // On failure returns false, writes the reason to `err`, and leaves `out` untouched.
    bool makeScenario(const Settings& settings, Maze& out, std::string* err = nullptr);

}  // namespace scenario