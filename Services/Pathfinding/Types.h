#pragma once

/// @brief Configuration types for the pathfinding system.

#include <Play.h>

namespace Pathfinding {

struct PathfindingConfig {

  // Top-left world-space origin of the playable area
  Play::Point2D playableOrigin{0,0};

  // Size of the playable area in world-space pixels
  Play::Point2D playableSize{0,0};

  // Tank body radius (px). Used to inflate obstacles for clearance.
  float tankRadius{20.0f};

  // Extra safety margin (px) added to the tankRadius for obstacle inflation.
  float safetyMargin{6.0f};

  // Pulls the graph inward from the outer boundary by this inset (px).
  float outerInset{0.0f};

  // Desired turning radius for motion primitives (px). Affects corner primitives
  float turnRadius{40.0f};
};

} // namespace Pathfinding