#include "IdleState.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Gateway/AIServiceGateway.h"

namespace AI {

void IdleState::OnEnter() {
  // Cleanup when entering Idle
  ctrl_.GW().CancelMove();
  ctrl_.GW().CancelAim();
  ctrl_.GW().ReleaseFire();
}

void IdleState::Tick(float /*dt*/) {
  if (!ctrl_.IsActive() || ctrl_.Self().hp <= 0) return; // stay idle
  ctrl_.ChangeState(FSMState::Patrol);
}

} // namespace AI

