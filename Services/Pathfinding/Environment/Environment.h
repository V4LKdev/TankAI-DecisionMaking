#pragma once

/// @brief Definitions and utilities for representing the pathfinding environment,
/// including playable area and obstacles.

#include <vector>
#include <Play.h>

namespace Pathfinding {

struct PathfindingConfig;

// Axisaligned rectangle in world space
struct Rect {
  float minx{};
  float miny{};
  float maxx{};
  float maxy{};
};

// Compute the inset outer playable rectangle based on Params
Rect GetOuterPlayableRect(const PathfindingConfig& params);

// True if point is strictly inside the inset outer rect (excludes boundary).
bool PointInOuterPlayable(const Play::Vector2D& point, const PathfindingConfig& params);

// Build inflated obstacle rectangles from current map structures and Params.
// Excludes the last structure (outer wall) by convention!
std::vector<Rect> BuildInflatedObstacles(const PathfindingConfig& params);

// Conservative rectangle helpers used across the system
// - PointInRect: inclusive boundaries, touching counts as inside
// - SegmentIntersectsRect: returns true on touching
// - SegmentHitsAnyRect: returns true if segment touches or crosses any rect in the list
bool PointInRect(const Play::Vector2D& point, const Rect& rect);
bool SegmentIntersectsRect(const Play::Vector2D& a, const Play::Vector2D& b, const Rect& rect);
bool SegmentHitsAnyRect(const Play::Vector2D& a, const Play::Vector2D& b, const std::vector<Rect>& rects);

} // namespace Pathfinding