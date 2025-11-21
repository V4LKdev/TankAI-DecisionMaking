#pragma once

/// @brief Types and configurations for Finite State Machine (FSM) AI controllers.

#include <cstdint>

namespace AI {

enum class FSMState : std::uint8_t {
  Idle,
  Patrol,
  LookAround,
  Engage,
  Search,
  Flee
};

struct FSMConfig {
  float patrolRepathSec{2.0f};
  float searchTimeoutSec{3.5f};
  float aimHoldMaxSec{1.5f};
  // Engage radii (hysteresis): attack when within attackRadius; chase when beyond chaseRadius
  float engageAttackRadius{240.0f};
  float engageChaseRadius{300.0f};
};

} // namespace AI
