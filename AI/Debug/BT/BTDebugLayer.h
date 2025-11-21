#pragma once

/// @brief Debug layer for Behavior Tree controllers: per-tank current node label.

#include "Debug/DebugLayer.h"

namespace AI {
class AISubsystem;
}

namespace AI::BT {
class BehaviorTreeController;
}

namespace AI {

class BTDebugLayer final : public DebugLayer {
public:
  void render(const AISubsystem& sys) const override;
  void handleInput(const AISubsystem& sys) override;
};

} // namespace AI

