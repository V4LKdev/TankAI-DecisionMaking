#include "GraphBuilder.h"
#include <algorithm>
#include <cmath>
#include "Helper/Geometry.h" // shared dist()

namespace Pathfinding {

void BuildCenterlineGraph(const Rect& outer, const std::vector<Rect>& obstacles, Graph& outGraph)
{
    outGraph.clear();

    // 1. Collect X/Y breakpoints from outer bounds and obstacles
    std::vector<float> xs{ outer.minx, outer.maxx };
    std::vector<float> ys{ outer.miny, outer.maxy };
    for (const auto& [minx, miny, maxx, maxy] : obstacles) {
        xs.push_back(minx); xs.push_back(maxx);
        ys.push_back(miny); ys.push_back(maxy);
    }

    // 2. Sort and remove near-duplicate breakpoints (within 1e-3)
    auto uniqSort = [](std::vector<float>& v) {
        std::ranges::sort(v);
        v.erase(std::ranges::unique(
                    v, [](float a, float b) { return std::abs(a - b) < 1e-3f; })
                    .begin(),
                v.end());
    };
    uniqSort(xs);
    uniqSort(ys);

    // 3. Compute centerlines (midpoints) between consecutive breakpoints.
    //    Only keep centerlines for gaps wider than 2.0 units.
    std::vector<float> centerXs; centerXs.reserve(xs.size());
    for (size_t i = 0; i + 1 < xs.size(); ++i) {
        const float a = xs[i], b = xs[i + 1];
        if (b - a > 2.0f) centerXs.push_back((a + b) * 0.5f);
    }

    std::vector<float> centerYs; centerYs.reserve(ys.size());
    for (size_t i = 0; i + 1 < ys.size(); ++i) {
        const float a = ys[i], b = ys[i + 1];
        if (b - a > 2.0f) centerYs.push_back((a + b) * 0.5f);
    }

    const int rows = static_cast<int>(centerYs.size());
    const int cols = static_cast<int>(centerXs.size());
    if (rows == 0 || cols == 0) return;

    // Map grid (row, col) -> node index in outGraph. Initialize to -1 (no node).
    std::vector<int> nodeIdx(rows * cols, -1);

    // 4. Place nodes at centerline intersections:
    //    - Skip points that fall inside any obstacle.
    //    - Skip points too close (<=1 unit) to the outer boundary.
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Play::Vector2D p{ centerXs[c], centerYs[r] };

            bool inside = false;
            for (const auto& rect : obstacles) {
                if (PointInRect(p, rect)) { inside = true; break; }
            }
            if (inside) continue;

            // keep nodes at least 1 unit away from outer boundary
            if (!(p.x > outer.minx + 1 && p.x < outer.maxx - 1 &&
                  p.y > outer.miny + 1 && p.y < outer.maxy - 1)) {
                continue;
            }

            nodeIdx[r * cols + c] = outGraph.addNode(p);
        }
    }

    // Helper to test if segment between two points intersects any obstacle.
    auto collides = [&](const Play::Vector2D& a, const Play::Vector2D& b) {
        return SegmentHitsAnyRect(a, b, obstacles);
    };

    // 5. Connect nodes horizontally within each row (left-to-right).
    for (int r = 0; r < rows; ++r) {
        int prevNode = -1;
        Play::Vector2D prevPos{};
        bool havePrev = false;

        for (int c = 0; c < cols; ++c) {
            const int idx = nodeIdx[r * cols + c];
            if (idx < 0) continue;

            const auto& pos = outGraph.nodes()[idx].pos;

            if (!havePrev) {
                // first node in this row
                prevNode = idx;
                prevPos = pos;
                havePrev = true;
            } else {
                // attempt to connect previous node to this one if no obstacle between them
                if (!collides(prevPos, pos)) {
                    const float cost = Geom::dist(prevPos, pos);
                    outGraph.addEdge(prevNode, idx, cost);
                }
                // advance previous
                prevNode = idx;
                prevPos = pos;
            }
        }
    }

    // 6. Connect nodes vertically within each column (top-to-bottom).
    for (int c = 0; c < cols; ++c) {
        int prevNode = -1;
        Play::Vector2D prevPos{};
        bool havePrev = false;

        for (int r = 0; r < rows; ++r) {
            const int idx = nodeIdx[r * cols + c];
            if (idx < 0) continue;

            const auto& pos = outGraph.nodes()[idx].pos;

            if (!havePrev) {
                prevNode = idx;
                prevPos = pos;
                havePrev = true;
            } else {
                if (!collides(prevPos, pos)) {
                    const float cost = Geom::dist(prevPos, pos);
                    outGraph.addEdge(prevNode, idx, cost);
                }
                prevNode = idx;
                prevPos = pos;
            }
        }
    }
}

} // namespace Pathfinding