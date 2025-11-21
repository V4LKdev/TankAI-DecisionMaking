#include "Services/Sensing/Vision/FOV.h"
#include "Services/Sensing/Vision/LOS.h"
#include "Data/AIContext.h"
#include <cmath>

namespace Sensing::Vision {

static inline float Dot(const Play::Vector2D& a, const Play::Vector2D& b) { return a.x*b.x + a.y*b.y; }

static inline float Len2(const Play::Vector2D& v) { return v.x*v.x + v.y*v.y; }

static inline Play::Vector2D Normalize(const Play::Vector2D& v) {
  const float l2 = Len2(v);
  if (l2 <= 1e-6f) return {0.f, 0.f};
  const float inv = 1.0f / std::sqrt(l2);
  return { v.x * inv, v.y * inv };
}

void FOV::Compute(const AI::SelfState& self,
                  const std::vector<AI::Contact>& candidates,
                  const LOS& /*los*/,
                  const SenseConfig& cfg,
                  std::vector<AI::Contact>& outVisible) {
  outVisible.clear();

  const float maxDist2 = cfg.viewDistance * cfg.viewDistance;
  const float halfFovRad = (cfg.fovDeg * 0.5f) * (3.14159265358979323846f / 180.0f);
  const float cosHalfFov = std::cos(halfFovRad);

  // Forward from rotation (assuming rot is radians; if degrees, convert here)
  const Play::Vector2D fwd{ std::cos(self.rot), std::sin(self.rot) };

  for (const auto& c : candidates) {
    const Play::Vector2D to = { c.pos.x - self.pos.x, c.pos.y - self.pos.y };
    const float d2 = Len2(to);
    if (d2 > maxDist2) continue; // distance cull

    const Play::Vector2D dir = Normalize(to);
    const float dot = Dot(dir, fwd);
    if (dot < cosHalfFov) continue; // cone cull

    // LOS check (call static helper)
    if (!LOS::HasLineOfSight(self.pos, c.pos)) continue;

    outVisible.push_back(c);
  }
}

} // namespace Sensing::Vision
