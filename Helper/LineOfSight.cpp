#include "Helper/LineOfSight.h"
#include "Obstacles/Structures.h"

namespace LOSHelper {

static inline bool SegmentIntersectsAARect(const Play::Vector2D& a, const Play::Vector2D& b, const Play::Point2D& bl, const Play::Point2D& size)
{
    // compute rect bounds in float
    const auto minx = static_cast<float>(bl.x);
    const auto miny = static_cast<float>(bl.y);
    const auto maxx = static_cast<float>(bl.x + size.x);
    const auto maxy = static_cast<float>(bl.y + size.y);

    // Liang-Barsky like test
    const float p[4] = { -(b.x - a.x), (b.x - a.x), -(b.y - a.y), (b.y - a.y) };
    const float q[4] = { a.x - minx, maxx - a.x, a.y - miny, maxy - a.y };
    float u1 = 0.0f, u2 = 1.0f;
    for (int i = 0; i < 4; ++i) {
        if (std::abs(p[i]) < 1e-6f) {
            if (q[i] < 0) return false; // parallel & outside
        } else {
            const float t = q[i] / p[i];
            if (p[i] < 0) { if (t > u2) return false; if (t > u1) u1 = t; }
            else          { if (t < u1) return false; if (t < u2) u2 = t; }
        }
    }
    return true;
}

bool HasLOS_RawStructures(const Play::Vector2D& a, const Play::Vector2D& b)
{
    // Use raw structures (non-inflated), skip last (outer wall) if present
    if (Structures.empty()) return true;
    for (size_t i = 0; i + 1 < Structures.size(); ++i) {
        const auto &[BottomLeft, Size] = Structures[i];
        if (SegmentIntersectsAARect(a, b, BottomLeft, Size)) {
            return false; // blocked
        }
    }
    return true;
}

} // namespace LOSHelper

