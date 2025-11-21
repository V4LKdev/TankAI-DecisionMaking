#pragma once

#include "FSMStateBase.h"
#include "AI/Controllers/FSM/FSMTypes.h"
#include <optional>
#include <Play.h>

namespace AI {

class EngageState final : public FSMStateBase {
public:
  enum class Mode : std::uint8_t { Chase, Attack };

  using FSMStateBase::FSMStateBase;
  void OnEnter() override;
  void Tick(float dt) override;
  void OnExit() override;

private:
  struct Resolved { Play::Vector2D pos; bool visible{}; };
  std::optional<Resolved> resolve_();

  // Runtime
  Mode mode_{ Mode::Chase };
  bool  rearmPending_{false};  // schedule BeginFire on next tick after a release
  float fireCooldown_{0.0f};   // cooldown between shots
};

} // namespace AI
