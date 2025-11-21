#pragma once

#include "FSMStateBase.h"

namespace AI {

class PatrolState final : public FSMStateBase {
public:
  using FSMStateBase::FSMStateBase;
  void OnEnter() override;
  void Tick(float dt) override;
};

} // namespace AI

