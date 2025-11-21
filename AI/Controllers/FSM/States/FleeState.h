#pragma once

#include "FSMStateBase.h"

namespace AI {

class FleeState final : public FSMStateBase {
public:
  using FSMStateBase::FSMStateBase;
  void OnEnter() override;
  void Tick(float dt) override;
  void OnExit() override;
  void OnDamageTaken();
  void OnThreatUpdate();

private:
  float timeSinceDamage_{0.0f};
  static constexpr float kFleeTimeoutSec = 5.0f; // Time without taking damage or being spotted to be considered "safe"
};

} // namespace AI
