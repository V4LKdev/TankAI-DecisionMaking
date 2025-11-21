#pragma once

/// @brief Motion primitives generation from polylines for pathfinding.

#include <vector>
#include <Play.h>

namespace Pathfinding {

struct PathfindingConfig;

// Basic motion primitive pieces

struct StraightPrim {
	Play::Vector2D a; // start point
	Play::Vector2D b; // end point
};

struct ArcPrim {
	Play::Vector2D center;
	float radius{0.0f};
	float startAngle{0.0f};
	float endAngle{0.0f};
	bool cw{false};         // true if the arc sweeps clockwise
	Play::Vector2D a;       // arc start point (tangent from previous primitive)
	Play::Vector2D b;       // arc end point (tangent to next primitive)
};

// Output container for a piecewise trajectory approximating a polyline
struct MotionPrimitives {
	std::vector<StraightPrim> straights;
	std::vector<ArcPrim> arcs;
};

// Optional statistics about placement decisions
struct MotionStats {
	int cornersConsidered{0};
	int primsPlaced{0};
	int primsRejectedClearance{0};
	int primsRejectedShort{0};
};

// Build motion primitives from a polyline using a target radius R.
// - Uses conservative clearance against inflated obstacles defined by Params.
// Returns true if primitives were built
bool BuildMotionPrimitives(const std::vector<Play::Vector2D>& polyline,
                           float R,
                           const PathfindingConfig& params,
                           MotionPrimitives& out,
                           MotionStats* stats = nullptr);

} // namespace Pathfinding
