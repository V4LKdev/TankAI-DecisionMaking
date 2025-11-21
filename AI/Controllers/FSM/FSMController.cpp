// ReSharper disable CppDFAUnreachableCode
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Controllers/FSM/States/FSMStates.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AI/Data/AIEvents.h"

namespace AI {

AIServiceGateway& FSMController::GW() { return gateway_; }

void FSMController::ChangeState(FSMState s) {
  if (stateId_ == s && current_) return;
  if (!idle_) {
    idle_   = std::make_unique<IdleState>(*this);
    patrol_ = std::make_unique<PatrolState>(*this);
    look_   = std::make_unique<LookAroundState>(*this);
    engage_ = std::make_unique<EngageState>(*this);
    search_ = std::make_unique<SearchState>(*this);
    flee_   = std::make_unique<FleeState>(*this);
  }
  if (current_) current_->OnExit();
  stateId_ = s;
  switch (s) {
    case FSMState::Idle:       current_ = idle_.get();   break;
    case FSMState::Patrol:     current_ = patrol_.get(); break;
    case FSMState::LookAround: current_ = look_.get();   break;
    case FSMState::Engage:     current_ = engage_.get(); break;
    case FSMState::Search:     current_ = search_.get(); break;
    case FSMState::Flee:       current_ = flee_.get();   break;
    default:                   current_ = idle_.get();   break;
  }
  if (current_) current_->OnEnter();
}

void FSMController::Update(float dt) {
  // Base hygiene (ensure subscriptions)
  AIDecisionController::Update(dt);

  gwSelfMirror_ = gateway_.Self();
  if (!current_) ChangeState(FSMState::Idle);
  if (!active_ || gwSelfMirror_.hp <= 0) { if (stateId_ != FSMState::Idle) ChangeState(FSMState::Idle); }
  if (current_) current_->Tick(dt);
}

void FSMController::onSpotted_(const SpottedEvent& e) {
  AIDecisionController::onSpotted_(e); // base sets target/lastKnown
  if (stateId_ == FSMState::Flee && flee_) {
    flee_->OnThreatUpdate();
  } else {
    ChangeState(FSMState::Engage);
  }
}
void FSMController::onLostSight_(const LostSightEvent& e) {
  AIDecisionController::onLostSight_(e); // base maintains lastKnown/clears

  // Do not force-leave Flee on lost sight
  if (stateId_ == FSMState::Flee && flee_) {
    return;
  }

  if (GetTargetId()) {
    // If sensing still has memory (base kept it), go Search; else LookAround
    if (GetLastKnown()) ChangeState(FSMState::Search); else ChangeState(FSMState::LookAround);
  }
}
void FSMController::onSound_(const SoundHeardEvent& e) {
  AIDecisionController::onSound_(e); // base updates lastKnownPos_
  if (stateId_ == FSMState::Flee && flee_) {
    flee_->OnThreatUpdate();
  } else if (stateId_ != FSMState::Engage) {
    ChangeState(FSMState::Search);
  }
}
void FSMController::onArrived_(const ArrivedEvent& /*e*/) {
  if (stateId_ == FSMState::Patrol || stateId_ == FSMState::Search) ChangeState(FSMState::LookAround);
}
void FSMController::onBlocked_(const BlockedEvent& /*e*/) {
  if (stateId_ == FSMState::Patrol) { ChangeState(FSMState::Patrol); return; }
  if (stateId_ == FSMState::Flee && flee_) { flee_->OnThreatUpdate(); }
}

void FSMController::onDamage_(int /*amount*/) {
  // Trigger flee state on damage
  if (stateId_ != FSMState::Flee) {
    ChangeState(FSMState::Flee);
  }
  // If already fleeing, notify the FleeState to reset its timer
  else if (stateId_ == FSMState::Flee && flee_) {
    flee_->OnDamageTaken();
  }
}

} // namespace AI
