#pragma once

/// @brief Types used by the Sensing service

#include <vector>
#include "Data/AIContext.h"

namespace Sensing {

enum class MemorySource { Vision, Hearing };

struct SenseConfig {
  float fovDeg        = 120.0f;
  float viewDistance  = 600.0f;
  
  float memoryTTL     = 5.0f;
};

struct PerceptionSnapshot {
  std::vector<AI::Contact> visible;
};

// Rich last-known info returned by sensing memory
struct LastKnownInfo {
  std::uint32_t id{0};
  Play::Vector2D pos{};
  float          ageSec{0.f};
  MemorySource   source{MemorySource::Vision};
  float          uncertaintyRadius{0.f}; // 0 for vision; >0 for hearing
};

// TODO: is the uncertainty and hearing radius really needed?

} // namespace Sensing
