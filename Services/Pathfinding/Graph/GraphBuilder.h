#pragma once

/// @brief Functions to build a rectilinear centerline graph for pathfinding.

#include <vector>
#include "Graph.h"
#include "Pathfinding/Environment/Environment.h" // Rect, collision helpers

namespace Pathfinding {

// Build the rectilinear centerline graph inside 'outer', avoiding 'obstacles'.
void BuildCenterlineGraph(const Rect& outer, const std::vector<Rect>& obstacles, Graph& outGraph);

} // namespace Pathfinding

