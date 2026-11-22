| Gap                                                 | Who covers it                      |
|-----------------------------------------------------|------------------------------------|
| No path (exit walled off) returns reachable = false | E                                  |
| Start equals exit returns [start] with cost 0       | E                                  |
| Invalid start id or empty graph                     | E                                  |
| Multiple exits (nearest one wins)                   | E, once you want the bonus feature |
| Large mazes, speed                                  | E's benchmarks                     |
| Diagonal mode costs                                 | Blocked by B's rounding issue      |

### Test Plan
| #  | Case                                                | Expected                                                             |
|----|-----------------------------------------------------|----------------------------------------------------------------------|
| 1  | Starts equals the exit                              | `reachable`, path `[start]`, cost 0                                  |
| 2  | Exit walled off or disconnected                     | Not reachable, empty path, `visitOrder` still filled                 |
| 3  | Start id -1, or id = node count                     | Not reachable, `nodesExpanded` 0                                     |
| 4  | Empty graph (0 vertices)                            | Not reachable                                                        |
| 5  | Invalid exit ids mixed with a valid one             | Valid one found, invalid ignored                                     |
| 6  | Empty exit list                                     | Not reachable                                                        |
| 7  | Two exits at different distances                    | The **nearest** twins                                                |
| 8  | Two equally short routes                            | The same one every run (deterministic)                               |
| 9  | The weighted smoke maze                             | Cost 8, path `0 5 10 11 12 13 14 9 4`                                |
| 10 | One-way edges (a directed grph built with `addEdge` | The path respects the direction                                      |
| 11 | `StopAtFirstTarget` vs `RunToEnd`                   | Same path and cost, and `nodesExpanded` of the first is ≤ the second |
| 12 | A 1001x1001 maze                                    | Finishes, cost = number of moves when unweighted                     | 

**The strongest test, a differential test:** generate many small random graphs 
with seeded randomness, solve each with our Dijkstra **and** with a slow but 
obviously correct algorithm (Bellman-Ford, a few lines), and require 
identical costs. If they agree on thousands of random cases, you can say in 
the Q&A "I verified it against an independent algorithm". That is much more 
convincing than a handful of examples.