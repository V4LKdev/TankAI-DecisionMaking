#include "MotionPrimitives.h"
#include <cmath>
#include <algorithm>
#include "Helper/Geometry.h"
#include "Pathfinding/Environment/Environment.h"

namespace Pathfinding {

// Local helpers

// Check if three points are nearly collinear by testing the cross product magnitude.
static bool isCollinear(const Play::Vector2D& a, const Play::Vector2D& b, const Play::Vector2D& c)
{
	const Play::Vector2D ab{ b.x - a.x, b.y - a.y };
	const Play::Vector2D bc{ c.x - b.x, c.y - b.y };
	return std::abs(Geom::cross(ab, bc)) < 1e-4f; // nearly collinear tolerance
}

// Remove intermediate points that lie nearly on the straight line between neighbors.
static void simplifyPolyline(const std::vector<Play::Vector2D>& in, std::vector<Play::Vector2D>& out)
{
	out.clear();
	if (in.empty()) return;

	// Always keep first point
	out.push_back(in.front());

	// Keep only non-collinear interior points
	for (size_t i = 1; i + 1 < in.size(); ++i) {
		if (!isCollinear(in[i - 1], in[i], in[i + 1])) out.push_back(in[i]);
	}

	// Always keep last point if there are at least two
	if (in.size() >= 2) out.push_back(in.back());
}

// Verify that an arc does not intersect obstacles and stays in playable area.
// TODO: is this function needed?
static bool arcClearanceOK(const ArcPrim& arc, const std::vector<Rect>& inflated, const PathfindingConfig& params)
{
	constexpr int samples = 24; // number of samples along the arc to check
	const Play::Vector2D ra{ arc.a.x - arc.center.x, arc.a.y - arc.center.y };
        const Play::Vector2D rb{ arc.b.x - arc.center.x, arc.b.y - arc.center.y };

	// starting angle of the arc and angle between the start/end radius vectors
        const float startAng = Geom::angOf(ra);
        const float angleBetween = std::acos(Geom::clampf((ra.x * rb.x + ra.y * rb.y) / (Geom::len(ra) * Geom::len(rb)), -1.0f, 1.0f));
        const float sweepAng = (arc.cw ? -1.0f : 1.0f) * angleBetween;

	// Sample points along the circular arc and test each point for validity
	for (int i = 0; i <= samples; ++i) {
		const float t = static_cast<float>(i) / samples;
		const float ang = startAng + sweepAng * t;
		Play::Vector2D p{ arc.center.x + arc.radius * std::cos(ang), arc.center.y + arc.radius * std::sin(ang) };

		// Must remain within the outer playable region
		if (!PointInOuterPlayable(p, params)) return false;

		// Must not intersect any inflated obstacle
		for (const auto& r : inflated) if (PointInRect(p, r)) return false;
	}

	return true;
}

// Compute intersection parameters (aOut and bOut) for two parametric lines:
// P + a*u  and  Q + b*v. Returns false if lines are parallel (no unique intersection).
static bool intersectLines(const Play::Vector2D& P, const Play::Vector2D& u,const Play::Vector2D& Q, const Play::Vector2D& v,float& aOut, float& bOut)
{
	const float denom = Geom::cross(u, v);
	if (std::abs(denom) < 1e-6f) return false; // parallel or nearly parallel

        const Play::Vector2D w{ Q.x - P.x, Q.y - P.y };
	aOut = Geom::cross(w, v) / denom;
	bOut = Geom::cross(w, u) / denom;
	return true;
}

// Build motion primitives (straight segments and circular arcs) from a polyline.
// - R: nominal turning radius
// - params: environment and playability parameters
// - out: resulting primitives
// - stats: optional statistics accumulator
bool BuildMotionPrimitives(const std::vector<Play::Vector2D>& polyline, const float R,const PathfindingConfig& params,MotionPrimitives& out,MotionStats* stats)
{
	out.straights.clear();
	out.arcs.clear();
	if (stats) *stats = {};
	if (polyline.size() < 2) return false;

	// Precompute inflated obstacles for clearance tests
	std::vector<Rect> inflated = BuildInflatedObstacles(params);

	// Simplify the input polyline to remove redundant collinear vertices
	std::vector<Play::Vector2D> pts;
	simplifyPolyline(polyline, pts);
	if (pts.size() < 2) return false;

	const int n = static_cast<int>(pts.size());

	// Store potential arc cuts at each interior vertex
	struct Cut { bool ok{false}; Play::Vector2D T1, T2; ArcPrim arc; };
	std::vector<Cut> cuts(n);

	// Iterate over interior corners to attempt to place an arc tangent to incoming/outgoing segments
	for (int i = 1; i <= n - 2; ++i) {
		if (stats) stats->cornersConsidered++;

		const Play::Vector2D prev = pts[i - 1];
		const Play::Vector2D cur  = pts[i];
		const Play::Vector2D next = pts[i + 1];

		// Tangent directions for incoming and outgoing segments (unit vectors)
		Play::Vector2D tanIn  = Geom::norm(Play::Vector2D{ cur.x - prev.x, cur.y - prev.y });
		Play::Vector2D tanOut = Geom::norm(Play::Vector2D{ next.x - cur.x, next.y - cur.y });

		// Angle between tangents; skip almost-straight or degenerate
                // corners
                const float dp = Geom::clampf(Geom::dot(tanIn, tanOut), -1.0f, 1.0f);
		float angleBetweenVectors = std::acos(dp);
		if (angleBetweenVectors < 1e-3f || std::isnan(angleBetweenVectors)) continue;

		// Determine turn direction (positive = left turn)
                const float turn = Geom::cross(tanIn, tanOut);
		if (std::abs(turn) < 1e-6f) continue;

		// Normal vectors pointing toward the center of a constant-radius arc depending on turn direction
		auto left  = [](const Play::Vector2D& t){ return Play::Vector2D{ -t.y, t.x }; };
		auto right = [](const Play::Vector2D& t){ return Play::Vector2D{ t.y, -t.x }; };
                const Play::Vector2D normIn  = (turn > 0.0f) ? left(tanIn) : right(tanIn);
                const Play::Vector2D normOut = (turn > 0.0f) ? left(tanOut) : right(tanOut);

		bool placed = false;

		// Non-strict mode: use nominal radius and accept if geometry and playability allow
		const float Rtry = R;
                auto P = Play::Vector2D{ cur.x + normIn.x * Rtry,  cur.y + normIn.y * Rtry };
                auto Q = Play::Vector2D{ cur.x + normOut.x * Rtry, cur.y + normOut.y * Rtry };

		float a, b;
		if (intersectLines(P, tanIn, Q, tanOut, a, b)) {
			const Play::Vector2D C{ P.x + tanIn.x * a, P.y + tanIn.y * a };

			const float s_in  = Geom::dot(Play::Vector2D{ C.x - cur.x, C.y - cur.y }, tanIn);
                        const float s_out = Geom::dot(Play::Vector2D{ C.x - cur.x, C.y - cur.y }, tanOut);
			Play::Vector2D T1{ cur.x + tanIn.x * s_in,  cur.y + tanIn.y * s_in };
			Play::Vector2D T2{ cur.x + tanOut.x * s_out, cur.y + tanOut.y * s_out };

			// Validate geometry and playability
			if (s_in < -1e-3f && s_out > 1e-3f && (-s_in) >= 2.0f && s_out >= 2.0f &&
				PointInOuterPlayable(T1, params) && PointInOuterPlayable(T2, params)) {

				ArcPrim arc;
				arc.center = C;
				arc.radius = Rtry;
				arc.a = T1;
				arc.b = T2;
				arc.cw = (turn < 0.0f);
				arc.startAngle = Geom::angOf(Play::Vector2D{ T1.x - C.x, T1.y - C.y });
				arc.endAngle   = Geom::angOf(Play::Vector2D{ T2.x - C.x, T2.y - C.y });

				cuts[i].ok = true;
				cuts[i].T1 = T1;
				cuts[i].T2 = T2;
				cuts[i].arc = arc;
				if (stats) stats->primsPlaced++;
				placed = true;
				}
		}
		if (!placed && stats) stats->primsRejectedShort++;
	}

	// Build straight primitives from remaining segments, applying cuts where arcs were placed
	for (int k = 0; k < n - 1; ++k) {
		Play::Vector2D S = pts[k];
		Play::Vector2D E = pts[k + 1];

		// If an arc starts at the end of this segment, trim E to the arc tangent point
		if (k + 1 <= n - 2 && cuts[k + 1].ok) E = cuts[k + 1].T1;

		// If an arc ends at the start of this segment, trim S to the arc tangent point
		if (k >= 1 && cuts[k].ok) S = cuts[k].T2;

		// Add short non-zero length straight segment
		if (Geom::len(Play::Vector2D{ E.x - S.x, E.y - S.y }) > 1e-3f) out.straights.push_back({ S, E });
	}

	// Append arcs in order
	for (int i = 1; i <= n - 2; ++i) if (cuts[i].ok) out.arcs.push_back(cuts[i].arc);

	return true;
}


} // namespace Pathfinding