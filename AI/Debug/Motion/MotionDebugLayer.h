#pragma once

/// @brief Debug layer for AI motion system.

#include "Debug/DebugLayer.h"

namespace AI {
class AISubsystem;

class MotionDebugLayer final : public DebugLayer {
public:
  void render(const AISubsystem& sys) const override;
  void handleInput(const AISubsystem& sys) override {}

private:
  // flash timers in seconds since last event
  mutable float arrivedFlash_{-1.0f};
  mutable float blockedFlash_{-1.0f};
};

} // namespace AI
