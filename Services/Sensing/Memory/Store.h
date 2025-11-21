#pragma once

/// @brief A simple store for last known positions of entities seen or heard by the AI.

#include <unordered_map>
#include <optional>
#include <cstdint>
#include <Play.h>
#include "Data/AIContext.h"
#include "Services/Sensing/Types.h"

namespace Sensing::Memory {

struct Entry {
  Play::Vector2D pos{};
  float          ageSec = 0.f;
  MemorySource   source{MemorySource::Vision};
  float          uncertaintyRadius{0.f};
};

class Store {
public:
  void RememberSeen (std::uint32_t id, const Play::Vector2D& pos);
  void RememberHeard(std::uint32_t id, const Play::Vector2D& pos, float uncertaintyRadius);
  void Forget(std::uint32_t id);
  std::optional<LastKnownInfo> LastKnown(std::uint32_t id) const;
  void Decay(float dt, float ttlSec);

private:
  std::unordered_map<std::uint32_t, Entry> seen_;
  std::unordered_map<std::uint32_t, Entry> heard_;
};

} // namespace Sensing::Memory
