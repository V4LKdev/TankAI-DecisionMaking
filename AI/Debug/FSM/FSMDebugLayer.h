#pragma once

/// @brief Debug layer for FSM controller: per-tank state label above each FSM-controlled tank.

#include "Debug/DebugLayer.h"

namespace AI {
class AISubsystem;

class FSMDebugLayer final : public DebugLayer {
public:
  void render(const AISubsystem& sys) const override;
  void handleInput(const AISubsystem& sys) override;
};

} // namespace AI
