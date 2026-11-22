
#pragma once

#include <cstddef>
#include <vector>

#include "Graph.h"

namespace dijkstra {

    // Total path cost. 64-bit so that long paths with big weights cannot overflow.
    using Cost = long long;

    // Everything the caller gets back from one search.
    struct Result {
        bool reachable = false;               // true if at least one target was reached
        Cost totalCost = 0;                   // cheapest cost to that target (meaningless if !reachable)
        std::vector<int> path;                // node ids, start first, target last (empty if !reachable)
        std::vector<int> visitOrder;          // node ids in the order they were finalised (for animation)
        std::size_t nodesExpanded = 0;        // how many nodes were finalised (for benchmarks)
    };

    // Controls whether the search stops early.
    enum class StopMode {
        StopAtFirstTarget,  // normal use: stop the moment a target is finalised
        RunToEnd            // benchmark use: keep going until every reachable node is finalised
    };

    // Finds the cheapest route from `start` to the nearest node in `targets`.
    // One exit is simply a list of one.
    //
    // Caller must guarantee (checked by assert in Debug builds only):
    //   - every edge weight is >= 0,
    //   - every edge points to a valid node id (0 .. numVertices() - 1),
    //   - the graph is not modified while the search runs.
    //
    // Behaviour:
    //   - Invalid `start` (< 0 or >= numVertices()): returns a default Result
    //     (reachable = false, everything empty, nodesExpanded = 0).
    //   - Invalid ids inside `targets` are ignored. If no valid target remains,
    //     the search explores everything reachable and reports reachable = false;
    //     visitOrder is still filled.
    //   - `path` and `totalCost` always describe the first target finalised
    //     (the nearest one), in both modes.
    //   - `visitOrder` lists nodes in the order they were finalised;
    //     nodesExpanded equals its size.
    //   - Deterministic: the same graph and arguments give the same Result.
    //     Ties are broken by node id in the queue, then by the graph's edge order.
    //
    // Cost: O((V + E) log V) time, O(V) extra memory.
    // Thread safety: reads the graph only, so it can run on a worker thread as long
    // as nobody changes that graph meanwhile.
    Result findShortestPath(const graph::Graph& g,
                            int start,
                            const std::vector<int>& targets,
                            StopMode mode = StopMode::StopAtFirstTarget);

}  // namespace dijkstra