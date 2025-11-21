#pragma once
#include <functional>

namespace Combat {

// Optional tuning
struct CombatConfig {
  // Reserved for future variables, Tank currently owns charge timing.
};

// Minimal fired event
using FiredCB = std::function<void()>;

} // namespace Combat
