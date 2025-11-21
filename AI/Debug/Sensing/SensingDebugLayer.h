#pragma once

/// @brief Debug layer for AI sensing system

#include "Debug/DebugLayer.h"
#include <string>
#include <vector>

namespace AI {
class AISubsystem;

class SensingDebugLayer final : public DebugLayer {
public:
  void render(const AISubsystem& sys) const override;
  void handleInput(const AISubsystem& sys) override;

private:
  // Scratch buffers to reduce per-frame allocations
  mutable std::string hudLine_;
  mutable std::vector<std::string> hudLines_;
};

} // namespace AI
