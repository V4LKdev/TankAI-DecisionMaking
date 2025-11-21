#include "PatrolState.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AI/Behaviors/CommonBehaviors.h"

namespace AI {

void PatrolState::OnEnter() {
  Behaviors::PatrolToRandomPoint(ctrl_.GW());
}

void PatrolState::Tick(const float dt) {
  (void)dt;
  if (!ctrl_.IsActive() || ctrl_.Self().hp <= 0) { ctrl_.ChangeState(FSMState::Idle); return; }
  if (ctrl_.GW().GetMotionStatus() != Motion::FollowCommand::Status::Following) {
    Behaviors::PatrolToRandomPoint(ctrl_.GW());
  }
}

} // namespace AI
