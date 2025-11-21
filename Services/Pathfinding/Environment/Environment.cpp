#include "Environment.h"
#include "Pathfinding/Types.h"
#include "Obstacles/Structures.h"
#include <algorithm>
#include <cmath>

namespace Pathfinding {
static Rect MakeRectInflated(const Structure& s, float inflate)
{
	Rect r;
	// Expand rectangle boundaries outward by inflation amount
	r.minx = static_cast<float>(s.BottomLeft.x) - inflate;
	r.miny = static_cast<float>(s.BottomLeft.y) - inflate;
	r.maxx = static_cast<float>(s.BottomLeft.x + s.Size.x) + inflate;
	r.maxy = static_cast<float>(s.BottomLeft.y + s.Size.y) + inflate;
	return r;
}

Rect GetOuterPlayableRect(const PathfindingConfig& params)
{
	// Clamp inset to nonnegative values to prevent expanding beyond boundaries
	const float inset = std::max(0.0f, params.outerInset);
	Rect r{};
	// Shrink playable area by inset amount on all sides
	r.minx = static_cast<float>(params.playableOrigin.x) + inset;
	r.miny = static_cast<float>(params.playableOrigin.y) + inset;
	r.maxx = static_cast<float>(params.playableOrigin.x + params.playableSize.x) - inset;
	r.maxy = static_cast<float>(params.playableOrigin.y + params.playableSize.y) - inset;
	return r;
}

bool PointInOuterPlayable(const Play::Vector2D& p, const PathfindingConfig& params)
{
	const auto [minx, miny, maxx, maxy] = GetOuterPlayableRect(params);
	// Exclude boundary points
	return (p.x > minx && p.x < maxx && p.y > miny && p.y < maxy);
}

std::vector<Rect> BuildInflatedObstacles(const PathfindingConfig& params)
{
	std::vector<Rect> obstacles;
	obstacles.reserve(Structures.size());

	// Inflate obstacles by tank radius plus safety margin to create collision boundaries
	const float inflate = params.tankRadius + params.safetyMargin;
	if (!Structures.empty()) {

		// Process all structures except the last one (outer wall)
		for (size_t i = 0; i + 1 < Structures.size(); ++i) {
			obstacles.push_back(MakeRectInflated(Structures[i], inflate));
		}
	}
	return obstacles;
}

bool PointInRect(const Play::Vector2D& point, const Rect& rect)
{
	// Use inclusive bounds to allow points on rectangle edges
	return (point.x >= rect.minx && point.x <= rect.maxx && point.y >= rect.miny && point.y <= rect.maxy);
}

bool SegmentIntersectsRect(const Play::Vector2D& a, const Play::Vector2D& b, const Rect& rect)
{
	// Liang-Barrsky algorithm: parametric representation where p[i] are direction components
	const float p[4] = { -(b.x - a.x), (b.x - a.x), -(b.y - a.y), (b.y - a.y) };

	// q[i] represents distances from point a to each rectangle boundary
	const float q[4] = { a.x - rect.minx, rect.maxx - a.x, a.y - rect.miny, rect.maxy - a.y };

	float u1 = 0.0f, u2 = 1.0f; // Parameter range [0,1] along segment

	for (int i = 0; i < 4; ++i) {
		// Check if segment is parallel to this boundary
		if (std::abs(p[i]) < 1e-6f) {
			if (q[i] < 0) return false; // Parallel and outside rectangle
		} else {
			const float t = q[i] / p[i];

			// Entering the half-plane defined by this boundary
			if (p[i] < 0) {
				if (t > u2) return false;
				if (t > u1) u1 = t;
			}

			// Leaving the half-plane defined by this boundary
			else {
				if (t < u1) return false;
				if (t < u2) u2 = t;
			}
		}
	}
	return true; // Segment intersects if u1 <= u2
}

bool SegmentHitsAnyRect(const Play::Vector2D& a, const Play::Vector2D& b, const std::vector<Rect>& rects)
{
	return std::ranges::any_of(
	rects, [&](const Rect& r) {
		return SegmentIntersectsRect(a, b, r); // Early exit on first collision
});
}

} // namespace Pathfinding
