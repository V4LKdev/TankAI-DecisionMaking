#pragma once

/// @brief A simple audio event bus for AI hearing simulation.

#include <vector>
#include <cstdint>
#include <Play.h>
#include "AI/Data/AIEvents.h"

namespace Sensing::Audio {

struct BusEvent {
  std::uint32_t             sourceId{0};
  Play::Vector2D            pos{};
  float                     radius{0.f};
  AI::SoundHeardEvent::Kind kind{AI::SoundHeardEvent::Kind::None};
  float                     ageSec{0.f};
  std::uint32_t             seq{0}; // unique sequence id assigned by Bus on push
};

class Bus {
public:
  void Push(const BusEvent& e);
  void Decay(float dt, float ttlSec = 1.5f);
  [[nodiscard]] std::vector<BusEvent> QueryInRadius(const Play::Vector2D& listenerPos) const;

  // Debug-only: return a snapshot of all current events (for overlays)
  [[nodiscard]] std::vector<BusEvent> Debug_All() const { return q_; }

private:
  std::vector<BusEvent> q_{};
  std::uint32_t seqCounter_{0};
};

} // namespace Sensing::Audio
