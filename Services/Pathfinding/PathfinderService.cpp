#include "PathfinderService.h"
#include "Obstacles/Structures.h"
#include "Environment/Environment.h"
#include "AStar/AStar.h"
#include "Graph/GraphBuilder.h"
#include "Helper/Geometry.h" // For Geom::dist, Geom::dist2, Geom::cross

#include <limits>
#include <algorithm>
#include <random>

namespace Pathfinding {

// --- Static Helper Functions ---

// Project point p onto segment a-b.
// Returns the projected point and writes the clamped parameter t in [0,1] into outT.
static Play::Vector2D project(const Play::Vector2D& p, const Play::Vector2D& a, const Play::Vector2D& b, float& outT) {

	// Direction vector of segment a->b
	const float dirX = b.x - a.x;
	const float dirY = b.y - a.y;

	// Vector from a to point p
	const float relX = p.x - a.x;
	const float relY = p.y - a.y;

	// Squared length of segment
	const float len2 = dirX*dirX + dirY*dirY;

	// Handle degenerate segment: return endpoint a
	if (len2 <= 1e-6f) {
		outT = 0.0f;
		return a; }

	// Projection parameter (not yet clamped)
	float t = (dirX*relX + dirY*relY) / len2;

	// Clamp to segment
	if (t < 0.0f) t = 0.0f;
	else if (t > 1.0f) t = 1.0f;

	outT = t;
	return Play::Vector2D{ a.x + t*dirX, a.y + t*dirY };
}

// Enumerate unique undirected edges from graph g.
// Returns pairs (u,v) with u < v to avoid duplicates.
static std::vector<std::pair<int,int>> listEdges(const Graph& g) {
	std::vector<std::pair<int,int>> edges;
	edges.reserve(g.size() * 3); // heuristic reserve: average degree ~3

	for (int u = 0; u < g.size(); ++u) {
		for (const auto &[to, cost] : g.nodes()[u].edges) {
			int v = to;
			// Only collect each undirected edge once (u < v)
			if (u < v) edges.emplace_back(u, v);
		}
	}
	return edges;
}

// Projects an arbitrary world point onto the base graph, returning the world position of the projection.
static Play::Vector2D ProjectPointToGraph(const Graph& base, const Play::Vector2D& point, const float snapDist)
{
    // 1. Try to snap to nearest existing node within snapDist
    float bestNodeD2 = snapDist * snapDist;
    for (int i = 0; i < base.size(); ++i) {
        const auto& q = base.nodes()[i].pos;
        const float d2 = Geom::dist2(point, q);
        if (d2 <= bestNodeD2) { return q; } // Snapped to a node, return its position
    }

    // 2. Otherwise find closest edge (by perpendicular distance) and project onto it
    float bestEdgeD2 = std::numeric_limits<float>::infinity();
    Play::Vector2D bestProjection = point; // Default to original point if no edge found

    auto edges = listEdges(base);
    for (auto [a, b] : edges) {
        const auto& Apos = base.nodes()[a].pos;
        const auto& Bpos = base.nodes()[b].pos;

        float t;
        Play::Vector2D projP = project(point, Apos, Bpos, t);
        const float d2 = Geom::dist2(point, projP);

        if (d2 < bestEdgeD2) {
            bestEdgeD2 = d2;
            bestProjection = projP;
        }
    }
    return bestProjection;
}

// Attaches a point to an augmented graph, returning the index of the new or snapped node.
static int AttachPointToGraph(const Graph& base,
                              Graph& augmented,
                              const Play::Vector2D& point,
                              const float snapDist)
{
    // 1. Try to snap to nearest existing node within snapDist
    int snappedNode = -1;
    float bestNodeD2 = snapDist * snapDist;
    for (int i = 0; i < base.size(); ++i) {
        const auto& q = base.nodes()[i].pos;
        const float d2 = Geom::dist2(point, q);
        if (d2 <= bestNodeD2) { bestNodeD2 = d2; snappedNode = i; }
    }
    if (snappedNode >= 0) return snappedNode;

    // 2. Otherwise find closest edge (by perpendicular distance) and split it
    float bestEdgeD2 = std::numeric_limits<float>::infinity();
    int bestA = -1, bestB = -1;
    Play::Vector2D bestProjection{};

    auto edges = listEdges(base);
    for (auto [a, b] : edges) {
        const auto& Apos = base.nodes()[a].pos;
        const auto& Bpos = base.nodes()[b].pos;

        float t;
        Play::Vector2D projP = project(point, Apos, Bpos, t);
        const float d2 = Geom::dist2(point, projP);

        if (d2 < bestEdgeD2) {
            bestEdgeD2 = d2;
            bestA = a;
            bestB = b;
            bestProjection = projP;
        }
    }

    if (bestA < 0 || bestB < 0) return -1;

    // 3. Insert new node at projection into `augmented` and replace the
    // undirected edge (bestA,bestB)
    const int newIdx = augmented.addNode(bestProjection);

    // Remove existing edge bestA -> bestB
    auto& edgesA = augmented.nodes()[bestA].edges;
    std::erase_if(edgesA, [&](const Edge &e) { return e.to == bestB; });

    // Remove existing edge bestB -> bestA
    auto& edgesB = augmented.nodes()[bestB].edges;
    std::erase_if(edgesB, [&](const Edge &e) { return e.to == bestA; });

    // Add split edges with accurate geometric costs
    const float costA = Geom::dist(base.nodes()[bestA].pos, bestProjection);
    const float costB = Geom::dist(bestProjection, base.nodes()[bestB].pos);
    augmented.addEdge(bestA, newIdx, costA);
    augmented.addEdge(newIdx, bestB, costB);

    return newIdx;
}

// --- PathfinderService Implementation ---

void PathfinderService::SetConfig(const PathfindingConfig& cfg)
{
    m_config = cfg;
}

const PathfindingConfig& PathfinderService::GetConfig() const
{
    return m_config;
}

void PathfinderService::Rebuild()
{
    m_graph.clear();

    if (Structures.empty())
        return;

    const Rect playArea = GetOuterPlayableRect(m_config);

    if (playArea.maxx <= playArea.minx + 4.0f || playArea.maxy <= playArea.miny + 4.0f)
        return; // inset too large; nothing to build

    const std::vector<Rect> inflatedObstacles = BuildInflatedObstacles(m_config);
    BuildCenterlineGraph(playArea, inflatedObstacles, m_graph);
}

std::optional<PathResult> PathfinderService::PlanPath(const Play::Vector2D& startPos, const Play::Vector2D& goalPos) const
{
    PathResult result;
    if (FindAttachedPath(startPos, goalPos, result)) {
        return result;
    }
    return std::nullopt;
}

Play::Vector2D PathfinderService::ProjectToWalkable(const Play::Vector2D& worldPos) const
{
    return ProjectPointToGraph(m_graph, worldPos, SNAP_DISTANCE);
}

Play::Vector2D PathfinderService::GetRandomReachablePoint() const
{
    if (m_graph.size() == 0) {
        return {0.f, 0.f};
    }

    // Get the playable area boundaries
    const auto [minx, miny, maxx, maxy] = GetOuterPlayableRect(m_config);

    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Generate random coordinates within the playable area
    std::uniform_real_distribution<float> distX(minx, maxx);
    std::uniform_real_distribution<float> distY(miny, maxy);

    const Play::Vector2D randomPoint = {distX(gen), distY(gen)};

    // Project this random point onto the nearest walkable part of the graph
    return ProjectToWalkable(randomPoint);
}

bool PathfinderService::IsReachable(const Play::Vector2D& startPos, const Play::Vector2D& goalPos) const
{
    return PlanPath(startPos, goalPos).has_value();
}

// --- Private Implementation ---

bool PathfinderService::FindAttachedPath(const Play::Vector2D& startPos, const Play::Vector2D& goalPos, PathResult& outResult) const
{
    outResult.polyline.clear();
    outResult.cost = 0.0f;

    if (m_graph.size() <= 0) return false;

    // Work on a cloned augmented graph so we don't modify the original base
    Graph augmented = m_graph;

    // Attach both endpoints to the augmented graph
    const int startIdx = AttachPointToGraph(m_graph, augmented, startPos, SNAP_DISTANCE);
    const int goalIdx  = AttachPointToGraph(m_graph, augmented, goalPos, SNAP_DISTANCE);
    if (startIdx < 0 || goalIdx < 0) return false;

    // Run pathfinding on augmented graph
    std::vector<int> nodePath;
    float pathCost = 0.0f;
    if (!FindPath(augmented, startIdx, goalIdx, nodePath, &pathCost)) return false;

    // Convert node indices to world-space points
    for (const int idx : nodePath) outResult.polyline.push_back(augmented.nodes()[idx].pos);
    outResult.cost = pathCost;
    return true;
}

} // namespace Pathfinding