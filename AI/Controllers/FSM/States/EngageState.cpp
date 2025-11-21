#include "EngageState.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "Helper/Geometry.h"
#include <algorithm>

namespace AI {

void EngageState::OnEnter() {
  ctrl_.GW().BeginFire();
  rearmPending_ = false;
  fireCooldown_ = 0.0f;
}

void EngageState::OnExit() { }

std::optional<EngageState::Resolved> EngageState::resolve_() {
  if (auto tid = ctrl_.GetTargetId()) {
    const auto visibles = ctrl_.GW().Sense_VisibleEnemies();
    for (const auto &c : visibles) { if (c.id == *tid) return Resolved{ c.pos, true }; }
  }
  return std::nullopt;
}

void EngageState::Tick(float dt) {
  if (!ctrl_.IsActive() || ctrl_.Self().hp <= 0) {
    ctrl_.GW().ReleaseFire();
    ctrl_.GW().CancelAim();
    ctrl_.GW().Stop();
    rearmPending_ = false;
    fireCooldown_ = 0.0f;
    ctrl_.ChangeState(FSMState::Idle); return;
  }

  // timers
  fireCooldown_ = std::max(0.0f, fireCooldown_ - dt);

  // Handle re-arm scheduled from last frame’s release
  if (rearmPending_) {
    ctrl_.GW().BeginFire();
    rearmPending_ = false;
  }

  auto r = resolve_();
  if (!r) {
    // Lost visibility: if we have last-known for current target, hand off to Search; else LookAround
    if (auto tid = ctrl_.GetTargetId()) {
      if (ctrl_.GW().Sense_LastKnown(*tid).has_value()) { ctrl_.ChangeState(FSMState::Search); return; }
    }
    ctrl_.GW().ReleaseFire();
    ctrl_.ChangeState(FSMState::LookAround);
    return;
  }

  const auto& cfg = ctrl_.Config();
  const auto selfPos = ctrl_.Self().pos;
  const float d = Geom::dist(selfPos, r->pos);

  // Base decision by distance hysteresis
  if (d > cfg.engageChaseRadius)      mode_ = Mode::Chase;
  else if (d <= cfg.engageAttackRadius) mode_ = Mode::Attack;
  else {/* keep mode */}

  if (mode_ == Mode::Chase) {
    ctrl_.GW().CancelAim();
    ctrl_.GW().MoveTo(r->pos);
    return;
  }

  // Attack
  ctrl_.GW().CancelMove();
  ctrl_.GW().AimAt(r->pos);

  // Use combat service accumulation to gate shot distance; map distance into desired charge fraction
  // his is just a nice to have and meant as a test.
  const float charge = ctrl_.GW().Combat_ChargeAccum();
  const float needed = std::clamp(d / std::max(1.0f, cfg.engageChaseRadius), 0.25f, 1.0f);
  const bool chargedEnough = charge >= needed;

  if (chargedEnough && fireCooldown_ <= 0.0f && ctrl_.GW().IsOnTarget() && ctrl_.GW().Sense_HasLOS(selfPos, r->pos)) {
    ctrl_.GW().ReleaseFire();        // falling edge -> shot
    rearmPending_ = true;            // re-arm next frame
    fireCooldown_ = 0.35f;           // small cadence
  }
}

} // namespace AI
