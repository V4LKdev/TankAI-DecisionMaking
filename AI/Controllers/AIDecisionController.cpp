#include "AIDecisionController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AI/Data/AIEvents.h"

namespace AI {

void AIDecisionController::EnsureSubscriptions() {
  if (subsBound_) return;
  AIServiceGateway::Subscriptions subs{};
  subs.onArrived   = [this](const ArrivedEvent& e){ onArrived_(e); };
  subs.onBlocked   = [this](const BlockedEvent& e){ onBlocked_(e); };
  subs.onSpotted   = [this](const SpottedEvent& e){ onSpotted_(e); };
  subs.onLostSight = [this](const LostSightEvent& e){ onLostSight_(e); };
  subs.onSound     = [this](const SoundHeardEvent& e){ onSound_(e); };
  subs.onDamage    = [this](const int amount){ onDamage_(amount); };
  gateway_.SetSubscriptions(subs);
  subsBound_ = true;
}

void AIDecisionController::Update(float /*deltaTime*/) {
  // Ensure event routing is connected; cheap no-op after first bind
  EnsureSubscriptions();
}

void AIDecisionController::SetActive(const bool on) {
  active_ = on;
  if (!active_) {
    gateway_.CancelMove();
    gateway_.CancelAim();
  }
}

void AIDecisionController::onSpotted_(const SpottedEvent& e) {
  targetId_ = e.id;
  if (auto lk = gateway_.Sense_LastKnown(e.id)) {
    lastKnownPos_ = lk->pos;
  }
}

void AIDecisionController::onLostSight_(const LostSightEvent& e) {
  if (targetId_ && *targetId_ == e.id) {
    // Keep last known if sensing still has memory, else clear target
    if (auto lk = gateway_.Sense_LastKnown(e.id)) {
      lastKnownPos_ = lk->pos;
    } else {
      targetId_.reset();
      lastKnownPos_.reset();
    }
  }
}

void AIDecisionController::onSound_(const SoundHeardEvent& e) {
  // Update last-known from sound; does not set a target id unless one already exists
  lastKnownPos_ = e.center;
}


} // namespace AI
