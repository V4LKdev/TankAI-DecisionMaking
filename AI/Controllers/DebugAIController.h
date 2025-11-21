#pragma once

/// @brief Debug AI controller for testing AI service gateway features

#include "AI/Controllers/AIDecisionController.h"

namespace AI {

class DebugAIController final : public AIDecisionController {
public:

  using AIDecisionController::AIDecisionController;

  void Update(float deltaTime) override;

private:
  bool wasAiming_{false};
  bool charging_{false};
  bool firedThisAim_{false};
  bool roamEnabled_{false};
};

} // namespace AI
