#pragma once

/// @brief Line of Sight (LOS) utility functions for vision sensing.

#include <Play.h>

namespace Sensing::Vision {

class LOS {
public:
  // Returns true if the segment a->b does not intersect any world obstacle rectangles.
  static bool HasLineOfSight(const Play::Vector2D& a, const Play::Vector2D& b);
};

} // namespace Sensing::Vision
