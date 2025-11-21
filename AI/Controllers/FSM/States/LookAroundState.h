#pragma once

#include "FSMStateBase.h"

namespace AI {

class LookAroundState final : public FSMStateBase {
public:
  using FSMStateBase::FSMStateBase;
  void OnEnter() override;
  void Tick(float dt) override;
private:
  float remaining_ = 0.0f; // radians
  float dirSign_   = 1.0f; // +1 ccw, -1 cw

  float minLookAngleDeg_ = 40.0f;
  float maxLookAngleDeg_ = 270.0f;
};

} // namespace AI
