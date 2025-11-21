#include "Services/Sensing/Audio/Bus.h"
#include <algorithm>
#include <cmath>

namespace Sensing::Audio {

void Bus::Push(const BusEvent& eIn) {
  BusEvent e = eIn;
  e.seq = ++seqCounter_;
  q_.push_back(e);
}

void Bus::Decay(const float dt, const float ttlSec) {
  for (auto &e : q_) e.ageSec += dt;
  std::erase_if(q_, [&](const BusEvent& e){ return e.ageSec > ttlSec; });
}

std::vector<BusEvent> Bus::QueryInRadius(const Play::Vector2D& listenerPos) const {
  std::vector<BusEvent> out;
  for (const auto &e : q_) {
    const float dx = e.pos.x - listenerPos.x;
    const float dy = e.pos.y - listenerPos.y;
    if (dx*dx + dy*dy <= e.radius * e.radius) out.push_back(e);
  }
  return out;
}

} // namespace Sensing::Audio
