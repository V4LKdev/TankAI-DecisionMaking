#include "FleeState.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Behaviors/CommonBehaviors.h"
#include "AI/Gateway/AIServiceGateway.h"

namespace AI {

void FleeState::OnEnter() {
  ctrl_.GW().ReleaseFire(); // ensure we stop firing while fleeing, this could result in an accidental kill, so we flee from nothing, but thats kinda funny
  ctrl_.GW().CancelAim();
  
  // Find a flee location using our reusable behavior
  const Play::Vector2D fleePoint = Behaviors::FindFleeLocation(
    ctrl_.Self().pos,
    ctrl_.GetLastKnown(),
    ctrl_.GW()
  );
  ctrl_.GW().MoveTo(fleePoint);

  timeSinceDamage_ = 0.0f; // Reset timer on entering flee
}

void FleeState::Tick(float dt) {
  // If not active or dead, go idle
  if (!ctrl_.IsActive() || ctrl_.Self().hp <= 0) {
    ctrl_.ChangeState(FSMState::Idle);
    return;
  }

  timeSinceDamage_ += dt;

  // Exit conditions:
  // 1. Arrived at flee point AND we've been fleeing for at least a short time
  // 2. Haven't taken damage for a long time (e.g., enemy gave up chase)
  const bool arrived = ctrl_.GW().GetMotionStatus() == Motion::FollowCommand::Status::Arrived;
  const bool safe = timeSinceDamage_ >= kFleeTimeoutSec;

  if (arrived || safe) {
    ctrl_.ChangeState(FSMState::LookAround); // Reassess situation
  }

  // TODO: If using low-level movement, handle getting stuck; FollowCommand covers Blocked.
}

void FleeState::OnExit() {
  ctrl_.GW().CancelMove(); // Ensure movement stops on exit
}
  // TODO: Add comment about reversing here for future enhancement
  // This is where we could implement low-level reversing logic if desired.

void FleeState::OnDamageTaken() {
// TODO: Redundant
  OnThreatUpdate();
}

void FleeState::OnThreatUpdate() {
  timeSinceDamage_ = 0.0f; // Reset timer on new threat (reinforce fleeing)
  const Play::Vector2D fleePoint = Behaviors::FindFleeLocation(
    ctrl_.Self().pos,
    ctrl_.GetLastKnown(),
    ctrl_.GW()
  );
  ctrl_.GW().MoveTo(fleePoint);
}

} // namespace AI
