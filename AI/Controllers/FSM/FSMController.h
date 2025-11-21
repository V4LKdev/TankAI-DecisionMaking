#pragma once

/// @brief Finite State Machine (FSM) AI controller implementation.

#include "AI/Controllers/AIDecisionController.h"
#include "AI/Controllers/FSM/FSMTypes.h"
#include "AI/Data/AIEvents.h"
#include "Data/AIContext.h"
#include "AI/Controllers/FSM/States/FSMStates.h"
#include <memory>
#include <optional>
#include <cstdint>

namespace AI {

class FSMStateBase;

class FSMController final : public AIDecisionController {
public:
  using AIDecisionController::AIDecisionController;

  void Update(float deltaTime) override;
  void SetActive(const bool on) override { AIDecisionController::SetActive(on); }

  // Debug accessor (state only)
  [[nodiscard]] FSMState Debug_State() const { return stateId_; }

  // State machine API used by states
  void ChangeState(FSMState s);
  [[nodiscard]] const FSMConfig& Config() const { return cfg_; }
  AIServiceGateway& GW();
  [[nodiscard]] const SelfState& Self() const { return gwSelfMirror_; }

  // Event routing from subsystem
  void onArrived_(const ArrivedEvent&) override;
  void onBlocked_(const BlockedEvent&) override;
  void onSpotted_(const SpottedEvent&) override;
  void onLostSight_(const LostSightEvent&) override;
  void onSound_(const SoundHeardEvent&) override;
  void onDamage_(int amount) override;

private:
  FSMState stateId_{FSMState::Idle};
  FSMConfig cfg_{};

  SelfState gwSelfMirror_{};

  std::unique_ptr<FSMStateBase> idle_;
  std::unique_ptr<FSMStateBase> patrol_;
  std::unique_ptr<FSMStateBase> look_;
  std::unique_ptr<EngageState>  engage_;
  std::unique_ptr<SearchState> search_;
  std::unique_ptr<FleeState> flee_;
  FSMStateBase* current_{nullptr};
};

} // namespace AI
