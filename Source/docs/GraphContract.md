# Graph Contract (B <-> C)

**Status:** Draft, agreed by Person C, waiting for Person B's review
**Owner of the implementation:** Person B (Graph Modeling)
**Main reader:** Person C (Dijkstra Core), then D and E
**Lives at:** `workflows/docs/graph-contract.md`
**Future header:** `include/graph/Graph.h` (implementation in `src/graph/Graph.cpp`)

## 1. Purpose

This file says what a "graph" means in this project, so B can build it and C can read it without talking to each other every day.

B turns a maze into a graph. C runs Dijkstra on the graph. Neither side should need to know how the other works inside.

## 2. Agreed decisions

| # | Decision | Agreed answer | Reason |
|---|---|---|---|
| 1 | Node identity | One integer per cell: `id = row * cols + col`. Reverse: `row = id / cols`, `col = id % cols`. | Dijkstra works with plain numbers, no coordinates needed. |
| 2 | Storage | Adjacency list: each node keeps a list of its outgoing edges. | A grid node has at most 4 neighbours. A matrix of V x V would be far too big for large mazes. |
| 3 | Edge content | Target node id and cost, as a pair `(to, weight)`. | An edge only needs to say where it leads and what it costs. |
| 4 | Weight type | Non-negative integer. | Dijkstra is not correct with negative weights. Integers avoid floating-point surprises. |
| 5 | Walls | A wall has **no edge**. It is never an "infinite cost" edge. | The algorithm never visits what has no road. |

## 3. What C needs from the graph (and nothing more)

- The number of nodes.
- For any node `u`, the list of its outgoing edges.

## 4. What the graph does NOT know

- Which cell is the start or the exit. The maze owns that, and `main.cpp` passes the two ids to Dijkstra.
- Anything about drawing, tests or the file format of a maze.

## 5. Who does what

| Task | Owner |
|---|---|
| Build the graph from a maze (create nodes and edges) | B |
| Provide `id <-> (row, col)` conversion helpers | B |
| Read the graph and compute shortest paths | C |
| Map path ids back to cells for drawing | D (using B's helpers) |
| Test that the graph is built correctly | E |

## 6. Open questions for B (please answer before coding)

| # | Question | Current assumption |
|---|---|---|
| Q1 | Moves in 4 directions (up, down, left, right) or 8 (with diagonals)? | 4 directions |
| Q2 | Is an edge cost "the cost of entering the target cell"? If cells can have different costs (mud, water), edges are effectively one-way, because A to B and B to A can cost differently. | Yes, cost = cost of entering the target cell |
| Q3 | What are the cell cost values for plain floor and for any extra terrain? | Plain floor = 1 |
| Q4 | How does B expose "neighbours of node u" to C (a list returned by reference, an iterator, ...)? | To be decided together before coding |

## 7. How to change this contract

1. Open a GitHub Issue describing the change and why.
2. B and C both approve in the Issue.
3. Update this file in the same pull request that changes `Graph.h`.

Nobody edits `Graph.h` in a way that breaks this file without step 1.