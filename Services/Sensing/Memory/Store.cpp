#include "Services/Sensing/Memory/Store.h"
#include <algorithm>

namespace Sensing::Memory {

void Store::RememberSeen(const std::uint32_t id, const Play::Vector2D& pos)  {
  seen_[id]  = Entry{pos, 0.f, MemorySource::Vision, 0.f};
}

void Store::RememberHeard(const std::uint32_t id, const Play::Vector2D& pos, const float uncertaintyRadius) {
  heard_[id] = Entry{pos, 0.f, MemorySource::Hearing, uncertaintyRadius};
}

void Store::Forget(const std::uint32_t id) {
  seen_.erase(id);
  heard_.erase(id);
}

std::optional<LastKnownInfo> Store::LastKnown(std::uint32_t id) const {
  const auto itSeen  = seen_.find(id);
  const auto itHeard = heard_.find(id);

  if (itSeen == seen_.end() && itHeard == heard_.end()) return std::nullopt;
  if (itSeen != seen_.end() && itHeard == heard_.end()) {
    return LastKnownInfo{ id, itSeen->second.pos, itSeen->second.ageSec, itSeen->second.source, itSeen->second.uncertaintyRadius };
  }
  if (itHeard != heard_.end() && itSeen == seen_.end()) {
    return LastKnownInfo{ id, itHeard->second.pos, itHeard->second.ageSec, itHeard->second.source, itHeard->second.uncertaintyRadius };
  }
  // Both present: select the fresher (smaller ageSec)
  const Entry& se = itSeen->second;
  const Entry& he = itHeard->second;
  if (se.ageSec <= he.ageSec) {
    return LastKnownInfo{ id, se.pos, se.ageSec, se.source, se.uncertaintyRadius };
  } else {
    return LastKnownInfo{ id, he.pos, he.ageSec, he.source, he.uncertaintyRadius };
  }
}

void Store::Decay(float dt, float ttlSec) {
  auto decayMap = [&](auto& m){
    for (auto& [id, e] : m) e.ageSec += dt;
    for (auto it = m.begin(); it != m.end(); ) {
      const bool expired = it->second.ageSec > ttlSec;
      if (expired) it = m.erase(it);
      else ++it;
    }
  };
  decayMap(seen_);
  decayMap(heard_);
}

} // namespace Sensing::Memory
