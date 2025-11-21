#include "PathfindingDebugLayer.h"

#include "AISubsystem.h"
#include "Globals.h"
#include "Pathfinding/Pathfinding.h"

namespace AI {

// Configuration constants used by the debug renderer.
static constexpr float kNodeRadius = 4.0f;
static constexpr int   kNodeLabelFont = 14;
static constexpr float kNodeLabelYOffset = 12.0f;
static constexpr float kStartGoalMarkerRadius = 6.0f;
static constexpr float kPointSnapEpsilon = 0.01f;
static constexpr float kArcSampleSpacing = 8.0f;
static constexpr int   kMinArcSteps = 4;
static constexpr float kSmallAngle = 1e-6f;

// --- Local helpers ---

// Draw simple path points and start/goal markers
static void DrawPathPoints(const std::vector<Play::Vector2D>& pts)
{
  if (pts.empty()) return;
  if (pts.size() == 1) {
    Play::DrawCircle(pts.front(), kStartGoalMarkerRadius, Play::cMagenta);
    return;
  }
  for (size_t i = 1; i < pts.size(); ++i) {
    Play::DrawLine(pts[i-1], pts[i], Play::cMagenta);
  }
  Play::DrawCircle(pts.front(), kStartGoalMarkerRadius, Play::cGreen);
  Play::DrawCircle(pts.back(),  kStartGoalMarkerRadius, Play::cRed);
}

// Normalize vector or default to unit X if very small
static Play::Vector2D NormalizeOrDefault(const Play::Vector2D& v) {
  const float L = std::sqrt(v.x*v.x + v.y*v.y);
  return (L > 1e-6f) ? Play::Vector2D{ v.x / L, v.y / L } : Play::Vector2D{ 1.0f, 0.0f };
}

// Append unique point based on small epsilon
static void AppendPointUnique(std::vector<Play::Vector2D>& out, const Play::Vector2D& p) {
  if (out.empty() || (std::fabs(out.back().x - p.x) > kPointSnapEpsilon || std::fabs(out.back().y - p.y) > kPointSnapEpsilon))
    out.push_back(p);
}

// Sample motion primitives into a polyline
static void SampleMotionPrimitivesToPolyline(const Pathfinding::MotionPrimitives& fp, std::vector<Play::Vector2D>& out)
{
  out.clear();
  const size_t maxCount = std::max(fp.straights.size(), fp.arcs.size());
  for (size_t i = 0; i < maxCount; ++i) {
    if (i < fp.straights.size()) {
      const auto &[a, b] = fp.straights[i]; // yes I like structured bindings :)
      AppendPointUnique(out, a);
      AppendPointUnique(out, b);
    }
    if (i < fp.arcs.size()) {
      const auto& a = fp.arcs[i];

      // vectors from center to arc endpoints
      const Play::Vector2D ra{ a.a.x - a.center.x, a.a.y - a.center.y };
      const Play::Vector2D rb{ a.b.x - a.center.x, a.b.y - a.center.y };
      const Play::Vector2D ua = NormalizeOrDefault(ra);
      const Play::Vector2D ub = NormalizeOrDefault(rb);

      // clamp dot product to avoid NaN from acos due to rounding
      const float dp = std::clamp(ua.x*ub.x + ua.y*ub.y, -1.0f, 1.0f);
      const float theta = std::acosf(dp);

      // if practically zero angle, just append endpoint
      if (theta < kSmallAngle) { AppendPointUnique(out, a.b); continue; }
      const float sign = a.cw ? -1.0f : 1.0f;
      const float a0 = std::atan2f(ra.y, ra.x);
      const float sweep = sign * theta;
      const float arcLen = std::fabs(sweep) * a.radius;
      const int steps = std::max(kMinArcSteps, static_cast<int>(arcLen / kArcSampleSpacing));

      for (int k = 0; k <= steps; ++k) {
        const float t = static_cast<float>(k) / static_cast<float>(steps);
        const float ang = a0 + sweep * t;
        Play::Vector2D pt{ a.center.x + a.radius * std::cosf(ang), a.center.y + a.radius * std::sinf(ang) };
        AppendPointUnique(out, pt);
      }
    }
  }
}


void PathfindingDebugLayer::render(const AISubsystem& sys) const {
    if (!visible_) {
        return;
    }

    const auto& pathfinder = sys.pathfinder();

    draw_graph(pathfinder);
    draw_path(pathfinder);
}

void PathfindingDebugLayer::handleInput(const AISubsystem& sys) {
  const auto& pathfinder = sys.pathfinder();
  const Play::Vector2D mousePos = Play::GetMousePos();

  if (Play::MousePressed(Play::MOUSE_LEFT))  {
    startPos_ = mousePos;
  }
  if (Play::MousePressed(Play::MOUSE_RIGHT)) {
    goalPos_ = mousePos;
  }

  if (Play::KeyPressed(Play::KEY_C)) {
    goalPos_ = pathfinder.GetRandomReachablePoint();
  }

  plan_and_store_path(pathfinder);


}

void PathfindingDebugLayer::draw_graph(const Pathfinding::PathfinderService& pathfinder) {

  const auto& g = pathfinder.GetGraph();

  // Draw edges
  for (int i = 0; i < g.size(); ++i) {
    const auto &[startPos, startEdges] = g.nodes()[i];
    for (const auto &[to, cost] : startEdges) {
      const auto &[endPos, endEdges] = g.nodes()[to];
      Play::DrawLine(startPos, endPos, Play::cCyan);
    }
  }
  // Draw nodes and their numeric labels on top of edges (or below)
  for (int i = 0; i < g.size(); ++i) {
    const auto &[pos, edges] = g.nodes()[i];
    Play::DrawCircle(pos, kNodeRadius, Play::cYellow);
    char label[32]{};
    std::snprintf(label, sizeof(label), "%d", i);
    Play::DrawDebugText({ pos.x, pos.y + kNodeLabelYOffset }, label, kNodeLabelFont, Play::cWhite);
  }
}

void PathfindingDebugLayer::draw_path(const Pathfinding::PathfinderService& pathfinder) const {
  // Draw start/goal markers if selected
  if (startPos_.has_value()) Play::DrawCircle(*startPos_, kStartGoalMarkerRadius, Play::cGreen);
  if (goalPos_.has_value())  Play::DrawCircle(*goalPos_,  kStartGoalMarkerRadius, Play::cRed);

  // Nothing else if no polyline
  if (pathPolyline_.empty()) return;

  // Draw polyline directly and try to draw a smoothed version via motion primitives
  // Build motion primitives
  Pathfinding::MotionPrimitives fp;
  Pathfinding::MotionStats st{};
  const auto& par = pathfinder.GetConfig();
  const bool okPrim = Pathfinding::BuildMotionPrimitives(pathPolyline_, par.turnRadius, par, fp, &st);
  const bool hasPrims = !fp.straights.empty() || !fp.arcs.empty();

  if (okPrim && hasPrims) {
    std::vector<Play::Vector2D> sampled;
    SampleMotionPrimitivesToPolyline(fp, sampled);
    if (!sampled.empty()) {
      DrawPathPoints(sampled);

      char label[64]{};
      std::snprintf(label, sizeof(label), "Cost: %.1f", pathCost_);
      Play::DrawDebugText({ pathPolyline_.back().x + 10.0f, pathPolyline_.back().y + 10.0f }, label, 14, Play::cWhite); // too lazy to write the + override lol

      return;
    }
  }

  // Fallback to drawing raw path
  DrawPathPoints(pathPolyline_);
}

void PathfindingDebugLayer::plan_and_store_path(const Pathfinding::PathfinderService& pathfinder) {

  if (startPos_.has_value() && goalPos_.has_value()) {
    if (const auto result = pathfinder.PlanPath(startPos_.value(), goalPos_.value()); result.has_value()){
      pathPolyline_ = result->polyline;
      pathCost_ = result->cost;
      return;
    }

    pathPolyline_.clear();
    pathCost_ = 0.0f;
  }
}

} // namespace AI