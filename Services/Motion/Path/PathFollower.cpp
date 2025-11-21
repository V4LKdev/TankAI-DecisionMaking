#include "Services/Motion/Path/PathFollower.h"
#include "Helper/Geometry.h"
#include <cmath>
#include <limits>

namespace Motion
{

// TODO: move to Geometry helper
static Play::Vector2D Lerp(const Play::Vector2D& a, const Play::Vector2D& b, const float t)
{
  return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

// Project point p to polyline; returns closest point; outputs segment index and param t in [0,1]
static Play::Vector2D ProjectToPolyline(const Play::Vector2D& p,const AI::Path& poly,int& outSeg,float& outT)
{
  float bestD2 = std::numeric_limits<float>::infinity();
  Play::Vector2D bestP{};
  int bestI = 0; float bestT = 0.0f;

  for (int i = 0; i + 1 < static_cast<int>(poly.size()); ++i)
  {
    const Play::Vector2D a = poly[i];
    const Play::Vector2D b = poly[i + 1];
    const Play::Vector2D ab{ b.x - a.x, b.y - a.y };
    const Play::Vector2D ap{ p.x - a.x, p.y - a.y };
    const float ab2 = ab.x * ab.x + ab.y * ab.y;

    float t = (ab2 > 1e-6f) ? ((ap.x * ab.x + ap.y * ab.y) / ab2) : 0.0f;
    t = std::clamp(t, 0.0f, 1.0f);
    const Play::Vector2D q{ a.x + ab.x * t, a.y + ab.y * t };
    const float d2 = Geom::dist2(p, q);

    if (d2 < bestD2) {
      bestD2 = d2;
      bestP = q;
      bestI = i;
      bestT = t;
    }
  }

  outSeg = bestI; outT = bestT; return bestP;
}

// Advance along polyline from segment i at param t by arc length s, return new position
static Play::Vector2D AdvanceAlongPolyline(const AI::Path& poly, const int seg, const float t, const float s)
{
  if (poly.empty()) return {};
  if (poly.size() == 1) return poly.front();

  int i = seg;
  float remaining = s;
  // Start position on segment i
  Play::Vector2D cur = (t > 1e-6f) ? Lerp(poly[i], poly[i + 1], t) : poly[i];

  while (remaining > 0.0f && i + 1 < static_cast<int>(poly.size()))
  {
    const Play::Vector2D a = cur;
    const Play::Vector2D b = poly[i + 1];
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float segLen = std::max(1e-6f, std::sqrt(dx*dx + dy*dy));

    if (remaining <= segLen)
    {
      const float r = remaining / segLen;
      return { a.x + (b.x - a.x) * r, a.y + (b.y - a.y) * r };
    }

    remaining -= segLen;
    ++i;
    if (i + 1 >= static_cast<int>(poly.size())) return poly.back();
    cur = poly[i];
  }

  return poly.back();
}

void PathFollower::SetPath(const AI::Path& p)
{
  path_ = p;
  currentSegmentIndex_ = 0;
  initialAlign_ = true; // enable one-time pre-alignment
}

bool PathFollower::HasPath() const { return !path_.empty(); }

void PathFollower::Cancel()
{
  path_.clear();
  currentSegmentIndex_ = 0;
}

FollowCommand PathFollower::Tick(const AI::SelfState& self)
{
  FollowCommand cmd;

  if (path_.empty()) {
    cmd.status = FollowCommand::Status::Idle;
    return cmd;
  }

  // Arrival tolerance
  const float arriveTol = profile_.arrive_tol_px;

  // Final goal check
  const Play::Vector2D goal = path_.back();
  const float dGoal = Geom::dist(self.pos, goal);

  if (dGoal <= arriveTol) {
    const Play::Vector2D toGoal{ goal.x - self.pos.x, goal.y - self.pos.y };
    const float desired = std::atan2f(toGoal.y, toGoal.x);
    const float alphaDock = Geom::wrapAngle(desired - self.rot);

    if (std::fabs(alphaDock) > profile_.ang_tol) {
      // rotate once toward goal, no forward move, this resutls in tank-like navigation
      cmd.rotate = (alphaDock > 0.0f ? -profile_.w_step : +profile_.w_step);
      cmd.move = 0.0f;
      cmd.status = FollowCommand::Status::Following;
      return cmd;
    }

    // Move just enough but not more than v_step to reduce residual distance
    cmd.move = std::min(profile_.v_step, dGoal);
    cmd.rotate = 0.0f;

    cmd.status = FollowCommand::Status::Arrived;
    return cmd;
  }

  // One-time pre-alignment: if starting far from goal and initial flag set, rotate on spot towards initial lookahead
  // TODO: add cost for large initial rotations? consider reversing if angle > 90deg?
  if (initialAlign_) {
    int segIdx0 = 0; float tOnSeg0 = 0.0f; (void)ProjectToPolyline(self.pos, path_, segIdx0, tOnSeg0);
    const float Lseed = std::max(profile_.lookahead_base, self.radius + 6.0f);
    const Play::Vector2D look0 = AdvanceAlongPolyline(path_, segIdx0, tOnSeg0, Lseed);
    const float desired0 = std::atan2f(look0.y - self.pos.y, look0.x - self.pos.x);
    const float alpha0 = Geom::wrapAngle(desired0 - self.rot);

    // TODO: could add some variance here to result in even better behavior
    if (std::fabs(alpha0) > profile_.ang_tol) {
      // Rotate only
      cmd.rotate = (alpha0 > 0.0f ? -profile_.w_step : +profile_.w_step);
      cmd.move = 0.0f;
      cmd.status = FollowCommand::Status::Following;
      return cmd;
    }
    // Heading is aligned sufficiently; begin normal following next tick
    initialAlign_ = false;
  }

  // Pure-pursuit style steering on the polyline
  int segIdx = 0; float tOnSeg = 0.0f;
  (void)ProjectToPolyline(self.pos, path_, segIdx, tOnSeg);

  // Base lookahead and constraints
  const float Rmin = std::max(1e-6f, profile_.v_step / std::max(1e-6f, profile_.w_step));
  const float Lmin = std::max(Rmin, self.radius + 6.0f);
  const float Lbase = std::max(profile_.lookahead_base, Lmin);
  const float Lmax = std::max(3.0f * Lbase, Lmin);

  // First guess lookahead using base value
  Play::Vector2D look = AdvanceAlongPolyline(path_, segIdx, tOnSeg, Lbase);
  float desired = std::atan2f(look.y - self.pos.y, look.x - self.pos.x);
  float alpha = Geom::wrapAngle(desired - self.rot);

  // Adaptive lookahead grows with heading error to prefer gentler curves
  const float L_eff = std::clamp(Lbase * (1.0f + profile_.k_alpha * std::fabs(alpha)), Lmin, Lmax);
  if (std::fabs(L_eff - Lbase) > 1e-3f) {
    look = AdvanceAlongPolyline(path_, segIdx, tOnSeg, L_eff);
    desired = std::atan2f(look.y - self.pos.y, look.x - self.pos.x);
    alpha = Geom::wrapAngle(desired - self.rot);
  }
  cmd.lookahead = look;

  // Steering: rotate at most one step in the needed direction, always try move forward one step
  if (std::fabs(alpha) > profile_.ang_tol) {
    cmd.rotate = (alpha > 0.0f ? -profile_.w_step : +profile_.w_step);
  } else {
    cmd.rotate = 0.0f;
  }
  cmd.move = profile_.v_step;

  cmd.status = FollowCommand::Status::Following;
  return cmd;
}

// Take a break from this mess of a code and look at this cute cat :)
// ⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡀⠀⠀⠀⠀
// ⠀⠀⠀⠀⢀⡴⣆⠀⠀⠀⠀⠀⣠⡀ ᶻ 𝗓 𐰁 .ᐟ ⣼⣿⡗⠀⠀⠀⠀
// ⠀⠀⠀⣠⠟⠀⠘⠷⠶⠶⠶⠾⠉⢳⡄⠀⠀⠀⠀⠀⣧⣿⠀⠀⠀⠀⠀
// ⠀⠀⣰⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣤⣤⣤⣤⣤⣿⢿⣄⠀⠀⠀⠀
// ⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣧⠀⠀⠀⠀⠀⠀⠙⣷⡴⠶⣦
// ⠀⠀⢱⡀⠀⠉⠉⠀⠀⠀⠀⠛⠃⠀⢠⡟⠀⠀⠀⢀⣀⣠⣤⠿⠞⠛⠋
// ⣠⠾⠋⠙⣶⣤⣤⣤⣤⣤⣀⣠⣤⣾⣿⠴⠶⠚⠋⠉⠁⠀⠀⠀⠀⠀⠀
// ⠛⠒⠛⠉⠉⠀⠀⠀⣴⠟⢃⡴⠛⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
// ⠀⠀⠀⠀⠀⠀⠀⠀⠛⠛⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀

} // namespace Motion
