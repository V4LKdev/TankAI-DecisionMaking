#pragma once

#include "FSMStateBase.h"
#include <optional>
#include <Play.h>

namespace AI {

class SearchState final : public FSMStateBase {
public:
  using FSMStateBase::FSMStateBase;
  void OnEnter() override;
  void Tick(float dt) override;
  void OnExit() override;
private:
  std::optional<Play::Vector2D> goal_{};
  bool rearmPending_{false};
  float timeout_{0.0f};
};

} // namespace AI

