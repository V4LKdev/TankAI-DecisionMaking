#include "SearchState.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Gateway/AIServiceGateway.h"

namespace AI {

void SearchState::OnEnter() {
  ctrl_.GW().BeginFire();
  rearmPending_ = false;
  timeout_ = 0.0f;

  goal_.reset();
  if (auto tid = ctrl_.GetTargetId()) {
    if (auto lk = ctrl_.GW().Sense_LastKnown(*tid)) goal_ = lk->pos;
  }
  if (!goal_) goal_ = ctrl_.GetLastKnown();

  if (goal_) {
    ctrl_.GW().MoveTo(*goal_);
  } else {
    ctrl_.GW().ReleaseFire();
    ctrl_.ChangeState(FSMState::LookAround);
  }
}

void SearchState::OnExit() { }

void SearchState::Tick(float dt) {
  if (!ctrl_.IsActive() || ctrl_.Self().hp <= 0) { ctrl_.GW().ReleaseFire(); ctrl_.ChangeState(FSMState::Idle); return; }

  // Reacquired → Engage
  if (auto tid = ctrl_.GetTargetId()) {
    const auto vis = ctrl_.GW().Sense_VisibleEnemies();
    for (const auto &c : vis) { if (c.id == *tid) { ctrl_.ChangeState(FSMState::Engage); return; } }
  }

  timeout_ += dt;
  if (timeout_ >= ctrl_.Config().searchTimeoutSec) { ctrl_.GW().ReleaseFire(); ctrl_.ChangeState(FSMState::LookAround); return; }

  if (ctrl_.GW().GetMotionStatus() == Motion::FollowCommand::Status::Following) {
    ctrl_.GW().CancelAim();
  }

  if (ctrl_.GW().GetMotionStatus() == Motion::FollowCommand::Status::Arrived) {
    ctrl_.GW().ReleaseFire();
    ctrl_.ChangeState(FSMState::LookAround);
  }
}

} // namespace AI
