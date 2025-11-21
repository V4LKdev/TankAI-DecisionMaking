#pragma once

/// @brief Line-of-sight utilities.

#include <Play.h>

namespace LOSHelper {

// Returns true if the segment a->b does NOT intersect any map structures (raw, non-inflated)
bool HasLOS_RawStructures(const Play::Vector2D& a, const Play::Vector2D& b);

} // namespace LOSHelper

