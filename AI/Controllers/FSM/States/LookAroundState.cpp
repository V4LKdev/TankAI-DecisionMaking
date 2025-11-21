#include "LookAroundState.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include <random>

namespace AI {

static thread_local std::mt19937 rngLA{ std::random_device{}() };

void LookAroundState::OnEnter() {
  ctrl_.GW().CancelMove();
  // choose degree and direction (converted to radians)
  constexpr float DegToRad = Play::PLAY_PI / 180.0f;
  std::uniform_real_distribution degDist(minLookAngleDeg_, maxLookAngleDeg_);
  remaining_ = degDist(rngLA) * DegToRad;
  std::bernoulli_distribution coin(0.5);
  dirSign_ = coin(rngLA) ? 1.0f : -1.0f;
}

void LookAroundState::Tick(float /*dt*/) {
  if (!ctrl_.IsActive() || ctrl_.Self().hp <= 0) { ctrl_.ChangeState(FSMState::Idle); return; }
  constexpr float delta = 0.05f; // fixed per-frame angle, TODO: should be set and managed by the motion service.
  const float step = std::min(delta, remaining_);
  ctrl_.GW().Turn(static_cast<int>(dirSign_)); // dirSign_ is 1.0f or -1.0f, cast to int for intent
  remaining_ -= step;
  if (remaining_ <= 1e-3f) ctrl_.ChangeState(FSMState::Patrol);
}

} // namespace AI

