#pragma once

/// @brief Implements A* pathfinding algorithm on a graph.

#include <vector>

namespace Pathfinding {
class Graph;

// Compute a path as a sequence of node indices from 'startNode' to 'goalNode'.
// Returns true if a path is found. 'outPath' is cleared and filled on success.
bool FindPath(const Graph& g, int startNode, int goalNode, std::vector<int>& outPath, float* outCost = nullptr);

} // namespace Pathfinding
