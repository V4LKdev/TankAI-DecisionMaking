#pragma once

/// @brief Converts planned paths into tank controls; follows paths, aims the turret, and reports arrival/blocked events.

#include "Motion/Types.h"
#include "Path/PathFollower.h"
#include <functional>
#include <optional>

class Tank;

namespace Motion
{

class MotionService {
public:
  // Bind a tank instance that this service will drive
  void BindTank(Tank* tank);

  // Configure motion profile (tuning parameters)
  void SetProfile(const MotionConfig& p);

  // Event callbacks for when a follow operation completes or gets blocked
  void SetCallbacks(const ArrivedCB &onArrived, const BlockedCB &onBlocked);

  // Set a callback to emit virtual movement sounds
  using SoundEmitCB = std::function<void(std::uint32_t /*sourceId*/, const Play::Vector2D& pos, float loudness)>;
  void SetSoundEmitter(const SoundEmitCB& cb) { soundEmit_ = cb; }

  // High level operations
  void FollowPath(const AI::Path& path);
  void CancelFollow();

  // Low level controls
  void Move(int intent) const;       // intent: -1 (backward), +1 (forward)
  void Rotate(int intent) const;     // intent: -1 (counterclockwise), +1 (clockwise)
  void Stop();

  // Aiming Intents
  void AimAt(const Play::Vector2D& target);
  void CancelAim();

  // Per-frame update called by owner
  void Tick(float dt, const AI::SelfState& self);

  // Aiming/Motion Queries
  [[nodiscard]] bool IsAiming() const;
  [[nodiscard]] bool IsOnTarget() const;
  [[nodiscard]] FollowCommand::Status GetStatus() const;

  // --- Debug accessors
  [[nodiscard]] std::optional<Play::Vector2D> Debug_GetGoal() const { return follower_.HasPath() ? std::optional<Play::Vector2D>(currentGoal_) : std::nullopt; }
  [[nodiscard]] std::optional<Play::Vector2D> Debug_GetAimTarget() const { return aimTarget_; }
  [[nodiscard]] std::optional<Play::Vector2D> Debug_GetLookahead() const { return lastLookahead_; }
  [[nodiscard]] float Debug_GetArriveBasePx() const { return profile_.arrive_tol_px; }

private:
  Tank* tank_{nullptr};
  MotionConfig profile_{};
  ArrivedCB onArrived_{};
  BlockedCB onBlocked_{};
  PathFollower follower_{};
  Play::Vector2D currentGoal_{};
  FollowCommand::Status lastStatus_{ FollowCommand::Status::Idle };

  // Aiming State
  std::optional<Play::Vector2D> aimTarget_{};

  Play::Vector2D lastProgressPos_{};
  int stuckCounter_{0};

  // Movement sound emission
  SoundEmitCB soundEmit_{};
  float soundTimer_{0.0f};
  Play::Vector2D lastSoundPos_{};
  const float moveEps_{2.0f};

  // Debug: store last computed lookahead point
  std::optional<Play::Vector2D> lastLookahead_{};
};
} // namespace Motion
