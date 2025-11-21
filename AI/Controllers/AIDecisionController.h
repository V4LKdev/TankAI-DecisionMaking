#pragma once

/// @brief Base class for AI decision controllers

#include "AI/Data/AIEvents.h"
#include <optional>
#include <cstdint>

namespace AI {

class AIServiceGateway;

class AIDecisionController {
public:
  explicit AIDecisionController(AIServiceGateway& gateway) : gateway_(gateway) {}
  virtual ~AIDecisionController() = default;

  // Base Update does framework hygiene (ensure event subscriptions). Derived updates
  // should call AIDecisionController::Update(deltaTime) first.
  virtual void Update(float deltaTime);

  // Activation hook
  virtual void SetActive(bool on);
  [[nodiscard]] virtual bool IsActive() const { return active_; }

  // Stable accessors for gameplay/overlay
  [[nodiscard]] std::optional<std::uint32_t> GetTargetId() const { return targetId_; }
  [[nodiscard]] std::optional<Play::Vector2D> GetLastKnown() const { return lastKnownPos_; }

protected:
  // One-time subscription binding; derived Update should call this each frame (cheap once-bound)
  void EnsureSubscriptions();

  // Event hooks (default: maintain target/last-known); derived may extend
  virtual void onArrived_(const ArrivedEvent&) {}
  virtual void onBlocked_(const BlockedEvent&) {}
  virtual void onSpotted_(const SpottedEvent& e);
  virtual void onLostSight_(const LostSightEvent& e);
  virtual void onSound_(const SoundHeardEvent& e);
  virtual void onDamage_(int amount) {}

protected:
  AIServiceGateway& gateway_;
  bool active_{false};
  bool subsBound_{false};

  // Shared knowledge
  std::optional<std::uint32_t> targetId_{};
  std::optional<Play::Vector2D> lastKnownPos_{};
};

} // namespace AI
