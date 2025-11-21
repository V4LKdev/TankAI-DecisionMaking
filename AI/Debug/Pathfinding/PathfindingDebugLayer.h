#pragma once

/// @brief Debug layer for visualizing pathfinding data.

#include "Debug/DebugLayer.h"

#include "Play.h"
#include <optional>
#include <vector>

namespace Pathfinding { class PathfinderService; }

namespace AI {

class PathfindingDebugLayer final : public DebugLayer {
public:
  void render(const AISubsystem& sys) const override;
  void handleInput(const AISubsystem& sys) override;

  private:
  std::optional<Play::Vector2D> startPos_;
  std::optional<Play::Vector2D> goalPos_;
  std::vector<Play::Vector2D> pathPolyline_;

  float pathCost_{0.0f};

  static void draw_graph(const Pathfinding::PathfinderService& pathfinder);
  void draw_path(const Pathfinding::PathfinderService& pathfinder) const;
  void plan_and_store_path(const Pathfinding::PathfinderService& pathfinder);
};

} // namespace AI