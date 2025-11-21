#include "AStar.h"
#include "Pathfinding/Graph/Graph.h"
#include <vector>
#include <queue>
#include <limits>
#include "Helper/Geometry.h"

namespace Pathfinding {

bool FindPath(const Graph& g, const int startNode, const int goalNode, std::vector<int>& outPath, float* outCost)
{
  outPath.clear();
  const int N = g.size();
  if (N <= 0) return false;
  if (startNode < 0 || goalNode < 0 || startNode >= N || goalNode >= N) return false;
  if (startNode == goalNode) {
    outPath.push_back(startNode);
    return true;
  }

  // Node entry stored in the open priority queue: node id and its f = g + h value
  struct Rec { int node; float f; };
  // Min-heap comparator (priority_queue is max-heap by default)
  auto cmp = [](const Rec& a, const Rec& b){ return a.f > b.f; };
  std::priority_queue<Rec, std::vector<Rec>, decltype(cmp)> open(cmp);

  // Cost from start to node (g), estimated total cost (f), and bookkeeping
  std::vector<float> gScore(N, std::numeric_limits<float>::infinity());
  std::vector<float> fScore(N, std::numeric_limits<float>::infinity());
  std::vector<int> cameFrom(N, -1);      // predecessor for path reconstruction
  std::vector<char> closed(N, 0);        // closed set marker (visited/expanded)

  // Heuristic: Euclidean distance from node n to the goal node (uses shared dist())
  auto h = [&](const int n){ return Geom::dist(g.nodes()[n].pos, g.nodes()[goalNode].pos); };

  // Initialize start node
  gScore[startNode] = 0.0f;
  fScore[startNode] = h(startNode);
  open.push({ startNode, fScore[startNode] });

  // Main A* loop
  while (!open.empty()) {
    // Pop the node with lowest f (note: entries can be stale; check closed[])
    const int current = open.top().node; open.pop();
    if (closed[current]) continue;       // already expanded with a better score
    closed[current] = 1;

    // If we reached the goal, reconstruct path by following cameFrom
    if (current == goalNode) {
      int n = current;
      while (n != -1) { outPath.push_back(n); n = cameFrom[n]; }
      std::ranges::reverse(outPath);
      if (outCost) *outCost = gScore[goalNode];
      return true;
    }

    // Relax neighbors
    for (const auto &[to, cost] : g.nodes()[current].edges) {
      const int nb = to;
      if (closed[nb]) continue;          // skip already expanded neighbor

      const float tentative = gScore[current] + cost;
      // Found a better path to neighbor
      if (tentative < gScore[nb]) {
        cameFrom[nb] = current;
        gScore[nb] = tentative;
        fScore[nb] = tentative + h(nb);
        open.push({ nb, fScore[nb] });  // push new entry
      }
    }
  }
  return false; // no path found
}
} // namespace Pathfinding