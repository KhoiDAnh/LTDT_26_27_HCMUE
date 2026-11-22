
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "Dijkstra.h"
#include "Graph.h"
#include "Maze.h"
#include "Scenario.h"

namespace {

using Clock = std::chrono::steady_clock;

// Mazes up to this many rows and columns are drawn in the console.
const int kMaxDrawSize = 60;

// Everything the command line can say.
struct Options {
    std::string file;                  // --file <path>
    bool useRandom = false;            // --random was given
    int randomSize = 0;                // --random <M>
    bool seedGiven = false;
    unsigned seed = 0;                 // --seed <N>
    scenario::MazeKind kind = scenario::MazeKind::Perfect;  // --kind
    double wallRatio = 0.3;            // --walls
    double weightRatio = 0.0;          // --weights
    int maxWeight = 5;                 // --max-weight
    bool endsGiven = false;            // --ends was given
    bool randomEnds = false;           // true = move Start/End randomly
};

// ---- text -> number helpers: return false instead of crashing on bad input ----

bool toInt(const std::string& text, int& value) {
    try {
        std::size_t used = 0;
        value = std::stoi(text, &used);
        return used == text.size();  // "12abc" is rejected
    } catch (const std::exception&) {
        return false;
    }
}

bool toUnsigned(const std::string& text, unsigned& value) {
    if (text.empty() || text[0] == '-') {
        return false;  // stoul would silently wrap a negative number
    }
    try {
        std::size_t used = 0;
        const unsigned long parsed = std::stoul(text, &used);
        if (used != text.size() || parsed > std::numeric_limits<unsigned>::max()) {
            return false;
        }
        value = static_cast<unsigned>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool toDouble(const std::string& text, double& value) {
    try {
        std::size_t used = 0;
        value = std::stod(text, &used);
        return used == text.size();
    } catch (const std::exception&) {
        return false;
    }
}

void printUsage(const char* program) {
    std::cout
        << "Usage:\n"
        << "  " << program << " --file <maze.txt> [options]\n"
        << "  " << program << " --random <M> [options]\n\n"
        << "Options:\n"
        << "  --seed <N>              seed for everything (random if omitted, and printed)\n"
        << "  --kind perfect|walls    generator for --random (default perfect)\n"
        << "  --walls <R>             wall ratio for --kind walls (default 0.3)\n"
        << "  --weights <R>           share of floor cells given a cost, 0..1 (default 0)\n"
        << "  --max-weight <W>        highest cell cost, 2..9 (default 5)\n"
        << "  --ends random|fixed     Start/End placement (default: random for --random,\n"
        << "                          fixed for --file)\n";
}

// Reads the command line into `o`. On bad input returns false and explains in `err`.
bool parseArgs(int argc, char** argv, Options& o, std::string& err) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (i + 1 >= argc) {  // every option takes exactly one value
            err = "Missing value after " + arg;
            return false;
        }
        const std::string value = argv[++i];

        if (arg == "--file") {
            o.file = value;
        } else if (arg == "--random") {
            if (!toInt(value, o.randomSize)) {
                err = "--random needs a whole number";
                return false;
            }
            o.useRandom = true;
        } else if (arg == "--seed") {
            if (!toUnsigned(value, o.seed)) {
                err = "--seed needs a non-negative whole number";
                return false;
            }
            o.seedGiven = true;
        } else if (arg == "--kind") {
            if (value == "perfect") {
                o.kind = scenario::MazeKind::Perfect;
            } else if (value == "walls") {
                o.kind = scenario::MazeKind::WallRatio;
            } else {
                err = "--kind must be perfect or walls";
                return false;
            }
        } else if (arg == "--walls") {
            if (!toDouble(value, o.wallRatio)) {
                err = "--walls needs a number";
                return false;
            }
        } else if (arg == "--weights") {
            if (!toDouble(value, o.weightRatio)) {
                err = "--weights needs a number";
                return false;
            }
        } else if (arg == "--max-weight") {
            if (!toInt(value, o.maxWeight)) {
                err = "--max-weight needs a whole number";
                return false;
            }
        } else if (arg == "--ends") {
            if (value == "random") {
                o.randomEnds = true;
            } else if (value == "fixed") {
                o.randomEnds = false;
            } else {
                err = "--ends must be random or fixed";
                return false;
            }
            o.endsGiven = true;
        } else {
            err = "Unknown option " + arg;
            return false;
        }
    }

    // Exactly one source of the maze.
    if (o.file.empty() == !o.useRandom) {
        err = "Give exactly one of --file or --random";
        return false;
    }

    // Default for Start/End: move them for generated mazes, keep them for files.
    if (!o.endsGiven) {
        o.randomEnds = o.useRandom;
    }
    return true;
}

double elapsedMs(Clock::time_point since) {
    return std::chrono::duration<double, std::milli>(Clock::now() - since).count();
}

// Runs one search, prints the numbers, and returns the result.
// `detailed` also prints the path and the visit order (only sensible for small mazes).
dijkstra::Result runAndReport(const graph::Graph& g, dijkstra::StopMode mode,
                              const char* label, bool detailed) {
    const std::vector<int> targets = {g.endVertex()};

    const Clock::time_point begin = Clock::now();
    const dijkstra::Result r =
        dijkstra::findShortestPath(g, g.startVertex(), targets, mode);
    const double ms = elapsedMs(begin);

    std::cout << "=== " << label << " ===\n";
    std::cout << "reachable     : " << (r.reachable ? "yes" : "no") << "\n";
    if (r.reachable) {
        std::cout << "totalCost     : " << r.totalCost << "\n";
        std::cout << "moves         : " << (r.path.size() - 1) << "\n";
    }
    std::cout << "nodesExpanded : " << r.nodesExpanded << "\n";
    std::cout << "search time   : " << ms << " ms\n";

    if (detailed) {
        std::cout << "path ids      :";
        for (int id : r.path) std::cout << ' ' << id;
        std::cout << "\nvisit order   :";
        for (int id : r.visitOrder) std::cout << ' ' << id;
        std::cout << "\n";
    }
    std::cout << "\n";
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    std::cout << std::fixed << std::setprecision(3);

    // 1. Read the command line.
    Options opt;
    std::string err;
    if (!parseArgs(argc, argv, opt, err)) {
        std::cerr << "Error: " << err << "\n\n";
        printUsage(argv[0]);
        return 1;
    }

    // 2. Seed: use the given one, or pick one and print it so the run can be repeated.
    if (!opt.seedGiven) {
        std::random_device device;
        opt.seed = device();
    }
    std::cout << "seed          : " << opt.seed << "\n";

    // 3. Get the maze, from a file or from the scenario builder.
    Maze maze;
    if (!opt.file.empty()) {
        if (!maze.loadFromFile(opt.file, &err)) {
            std::cerr << "Cannot load maze: " << err << "\n";
            return 1;
        }
        if (opt.randomEnds && !scenario::randomizeStartEnd(maze, opt.seed, &err)) {
            std::cerr << "Cannot randomize Start/End: " << err << "\n";
            return 1;
        }
    } else {
        scenario::Settings settings;
        settings.size = opt.randomSize;
        settings.seed = opt.seed;
        settings.kind = opt.kind;
        settings.wallRatio = opt.wallRatio;
        settings.weightRatio = opt.weightRatio;
        settings.maxWeight = opt.maxWeight;
        settings.randomStartEnd = opt.randomEnds;
        if (!scenario::makeScenario(settings, maze, &err)) {
            std::cerr << "Cannot build scenario: " << err << "\n";
            return 1;
        }
    }

    // 4. Show what we really got (A's generators may round or fall back silently).
    maze.printStats(std::cout);
    std::cout << "\n";

    // 5. Build the graph (4 directions) and time it.
    const Clock::time_point buildStart = Clock::now();
    const graph::Graph g = graph::Graph::fromMaze(maze);
    const double buildMs = elapsedMs(buildStart);
    std::cout << "graph build   : " << buildMs << " ms (" << g.numVertices()
              << " vertices, " << g.numEdges() << " edges)\n\n";

    // 6. Start and End must be on walkable cells.
    if (g.startVertex() < 0 || g.endVertex() < 0) {
        std::cerr << "Start or End is not on a walkable cell.\n";
        return 1;
    }

    // 7. Solve twice: stop at the exit, and run to the end.
    const bool small = maze.rows() <= kMaxDrawSize && maze.cols() <= kMaxDrawSize;
    const dijkstra::Result first = runAndReport(
        g, dijkstra::StopMode::StopAtFirstTarget, "StopAtFirstTarget", small);
    runAndReport(g, dijkstra::StopMode::RunToEnd, "RunToEnd", small);

    // 8. Draw the maze with the route, only if it fits on screen.
    if (small) {
        std::vector<Position> route;
        route.reserve(first.path.size());
        for (int id : first.path) route.push_back(g.toPosition(id));
        std::cout << maze.toStringWithPath(route);
    }
    return 0;
}