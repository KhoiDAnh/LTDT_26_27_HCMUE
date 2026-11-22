#include "Dijkstra.h"

#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>


namespace dijkstra {

    namespace {  // private to this file, nobody else can see these

        // "Not reached yet". Never add to this value: it would overflow.
        const Cost kInfinity = std::numeric_limits<Cost>::max();

        // "This node has no parent (yet)".
        const int kNoParent = -1;

        // One heap entry: (distance at the time of pushing, node id).
        using HeapEntry = std::pair<Cost, int>;

        // Min-heap: the smallest distance is always on top.
        using MinHeap =
            std::priority_queue<HeapEntry, std::vector<HeapEntry>, std::greater<HeapEntry>>;

    }  // namespace

    Result findShortestPath(const graph::Graph& g,
                            int start,
                            const std::vector<int>& targets,
                            StopMode mode) {
        Result result;

        // ---- Chunk 1: preparation ----
        const int nodeCount = g.numVertices();

        // Invalid start: nothing to search, report "not reachable".
        if (start < 0 || start >= nodeCount) {
            return result;
        }

        // isTarget[u] answers "is u an exit?" in O(1).
        std::vector<char> isTarget(nodeCount, 0);
        for (int t : targets) {
            if (t >= 0 && t < nodeCount) {
                isTarget[t] = 1;
            }
        }

        std::vector<Cost> dist(nodeCount, kInfinity);  // best known cost to each node
        std::vector<int> parent(nodeCount, kNoParent);  // who we came from, for the path
        MinHeap heap;

        dist[start] = 0;
        heap.emplace(0, start);

        // ---- Chunk 2: main loop goes here ----
        int reachedTarget = -1;  // first exit finalised; -1 = none yet

        while (!heap.empty()) {
            const HeapEntry top = heap.top();  // copy first, pop() invalidates a reference
            heap.pop();
            const Cost d = top.first;
            const int u = top.second;

            // Stale entry: a cheaper route to u was found after this one was pushed.
            if (d > dist[u]) {
                continue;
            }

            // u is final now.
            result.visitOrder.push_back(u);
            ++result.nodesExpanded;

            // First exit finalised?
            if (isTarget[u] && reachedTarget == -1) {
                reachedTarget = u;
                if (mode == StopMode::StopAtFirstTarget) {
                    break;
                }
            }

            // Relax every outgoing edge of u.
            for (const graph::Edge& e : g.neighbors(u)) {
                assert(e.weight >= 0 && "Dijkstra needs non-negative edge weights");
                assert(e.to >= 0 && e.to < nodeCount && "edge points outside the graph");
                const Cost newCost = d + e.weight;
                if (newCost < dist[e.to]) {
                    dist[e.to] = newCost;
                    parent[e.to] = u;
                    heap.emplace(newCost, e.to);
                }
            }
        }
        // ---- Chunk 3: path rebuild (right after the closing brace of the while loop) ----
        if (reachedTarget == -1) {
            return result;  // no exit reached: reachable stays false, path stays empty
        }

        result.reachable = true;
        result.totalCost = dist[reachedTarget];

        // Follow the breadcrumbs from the exit back to the start (this collects them backwards).
        for (int node = reachedTarget; node != kNoParent; node = parent[node]) {
            result.path.push_back(node);
        }
        std::reverse(result.path.begin(), result.path.end());  // now start -> exit

        return result;
    }

}  // namespace dijkstra