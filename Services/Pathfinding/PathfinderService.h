#pragma once

/// @brief Builds a simple nav graph and serves path queries (plan/project/reachability) for the tank arena.

#include "Graph/Graph.h"
#include "Types.h"
#include <Play.h>
#include <vector>
#include <optional>

namespace Pathfinding {

// A struct to hold the result of a path query, including the path itself and its total cost.
struct PathResult {
    std::vector<Play::Vector2D> polyline;
    float cost{};
};

class PathfinderService {
public:
    PathfinderService() = default;

    // --- Configuration & Setup ---

    // Sets the configuration (tank size, margins) and must be called before Rebuild.
    void SetConfig(const PathfindingConfig& cfg);
    [[nodiscard]] const PathfindingConfig& GetConfig() const;

    // Builds the internal navigation graph.
    void Rebuild();

    // Returns the internal navigation graph.
    [[nodiscard]] const Graph& GetGraph() const { return m_graph; }

    // --- Core AI Queries ---

    // Plans a path from a start to a goal, returning the path and its cost.
    // This is the primary function for movement planning.
    [[nodiscard]] std::optional<PathResult> PlanPath(const Play::Vector2D& startPos, const Play::Vector2D& goalPos) const;

    // Projects an arbitrary point to the nearest valid "walkable" location on the nav graph.
    [[nodiscard]] Play::Vector2D ProjectToWalkable(const Play::Vector2D& worldPos) const;

    // Returns a random, valid, reachable point on the map.
    [[nodiscard]] Play::Vector2D GetRandomReachablePoint() const;
    // TODO: Add version with area constraints or radius around point

    // Checks if a path exists between two points. Cheaper than planning the full path.
    [[nodiscard]] bool IsReachable(const Play::Vector2D& startPos, const Play::Vector2D& goalPos) const;

private:
    // --- Internal Implementation ---

    // Finds a path using temporary graph attachments. The core of PlanPath.
    bool FindAttachedPath(const Play::Vector2D& startPos, const Play::Vector2D& goalPos, PathResult& outResult) const;

    PathfindingConfig m_config{};
    Graph m_graph{};

    // Snap distance used for attaching points to the graph.
    static constexpr float SNAP_DISTANCE = 24.0f;
};

} // namespace Pathfinding