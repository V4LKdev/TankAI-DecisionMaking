#include "Services/Motion/MotionService.h"
#include "CoreTank/Tank.h"
#include "AI/Data/AIContext.h"
#include "Helper/Geometry.h"
#include "Globals.h"
#include <cmath>

namespace Motion
{

namespace {
  // Unified audio emission cadence and loudness levels
  static constexpr float kEnginePeriodSec = 1.0f;   // one event per second
  static constexpr float kDrivingLoud     = 150.0f;  // radius when moving
  static constexpr float kIdleLoud        = 75.0f;  // radius when stationary
}

void MotionService::BindTank(Tank* tank) { tank_ = tank; lastSoundPos_ = tank ? tank->GetPosition() : Play::Vector2D{}; }

void MotionService::SetProfile(const MotionConfig& p) { profile_ = p; follower_.SetProfile(p); }

void MotionService::SetCallbacks(const ArrivedCB &onArrived, const BlockedCB &onBlocked) { onArrived_ = onArrived; onBlocked_ = onBlocked; }

void MotionService::FollowPath(const AI::Path& path)
{
  follower_.SetProfile(profile_);
  follower_.SetPath(path);

  if (!path.empty())
  {
    currentGoal_ = path.back();
  }

  // Reset event and progress state when a new path is issued
  lastStatus_ = FollowCommand::Status::Idle;
  stuckCounter_ = 0;
  lastLookahead_.reset();
  if (tank_) lastProgressPos_ = tank_->GetPosition();
}

void MotionService::CancelFollow()
{
  follower_.Cancel();
  lastStatus_ = FollowCommand::Status::Idle;
  stuckCounter_ = 0;
  lastLookahead_.reset();
}

void MotionService::Move(const int intent) const { if (tank_) tank_->Move(intent * TANK_MOVE_SPEED); }

void MotionService::Rotate(const int intent) const { if (tank_) tank_->Rotate(intent * TANK_ROTATION_SPEED); }

void MotionService::Stop()
{
  // Clear any following state and ensure the tank is halted this frame
  CancelFollow();
  if (tank_) tank_->Move(0.f);
}

void MotionService::AimAt(const Play::Vector2D& target) {
    aimTarget_ = target;
    // When aiming, cancel any path following as aiming takes precedence for rotation
    follower_.Cancel();
    lastStatus_ = FollowCommand::Status::Idle; // Reset status
}

void MotionService::CancelAim() {
    aimTarget_.reset();
}

void MotionService::Tick(const float dt, const AI::SelfState& self)
{
  if (!tank_) return;

  soundTimer_ += dt;

  const auto [move, rotate, status, lookahead] = follower_.Tick(self);
  lastLookahead_ = follower_.HasPath() ? std::optional<Play::Vector2D>(lookahead) : std::nullopt;

  float final_rotate_command = rotate; // Default to path follower's rotation
  if (IsAiming()) {
      const Play::Vector2D to_target = {aimTarget_->x - tank_->GetPosition().x, aimTarget_->y - tank_->GetPosition().y};
      const float desired_angle = Geom::angOf(to_target);
      const float current_angle = tank_->GetRotation();
      const float angle_diff = Geom::wrapAngle(desired_angle - current_angle);

      if (std::fabs(angle_diff) > profile_.ang_tol) { // Only rotate if not already on target
          final_rotate_command = (angle_diff > 0.0f ? -profile_.w_step : +profile_.w_step);
      } else {
          final_rotate_command = 0.0f; // On target, no rotation needed
      }
  }

  // Apply movement commands produced by the follower
  const Play::Vector2D before = tank_->GetPosition();
  if (final_rotate_command != 0.0f) { tank_->Rotate(final_rotate_command); }
  if (move   != 0.0f) { tank_->Move(move); }
  const Play::Vector2D after = tank_->GetPosition();

  // Emit virtual engine/hum sound on a single cadence
  if (soundEmit_)
  {
    const float movedSinceLastEmit = Geom::dist(lastSoundPos_, after);

    if (soundTimer_ >= kEnginePeriodSec)
    {
      soundTimer_ = 0.0f;
      lastSoundPos_ = after;
      const bool wasMoving = (movedSinceLastEmit > moveEps_);
      const float loud = wasMoving ? kDrivingLoud : kIdleLoud;
      const auto sid = static_cast<std::uint32_t>(tank_->GetID());
      soundEmit_(sid, after, loud);
    }
  }

  // Handle arrival: notify once when status transitions into Arrived
  if (status == FollowCommand::Status::Arrived && lastStatus_ != FollowCommand::Status::Arrived)
  {
    if (onArrived_) onArrived_(currentGoal_);
    follower_.Cancel();
    lastStatus_ = FollowCommand::Status::Arrived;
    stuckCounter_ = 0;
    lastLookahead_.reset();
    lastProgressPos_ = tank_->GetPosition();
    return;
  }

  // Progress detection: only consider forward commands (move) as progress attempts
  const float moved = Geom::dist(before, after);
  if (std::abs(move) > 1e-6f)
  {
    if (moved <= profile_.progress_eps)
    {
      ++stuckCounter_;
      if (stuckCounter_ >= profile_.stuck_frames)
      {
        // Consider blocked: notify and clear follow
        follower_.Cancel();
        if (onBlocked_) onBlocked_(after);
        lastStatus_ = FollowCommand::Status::Blocked;
        stuckCounter_ = 0;
        lastLookahead_.reset();
        return;
      }
    }
    else
    {
      // made progress -> reset anchor and counter
      stuckCounter_ = 0;
      lastProgressPos_ = after;
    }
  }

  // Reflect state transitions for non-terminal statuses
  if (status != lastStatus_)
  {
    lastStatus_ = status;
  }
  else
  {
    // If we had recently been Arrived but follower has no path, transition to Idle
    if (lastStatus_ == FollowCommand::Status::Arrived && !follower_.HasPath())
    {
      lastStatus_ = FollowCommand::Status::Idle;
    }
  }
}

bool MotionService::IsAiming() const {
    return aimTarget_.has_value();
}

bool MotionService::IsOnTarget() const {
    if (!tank_ || !aimTarget_.has_value()) {
        return false;
    }
    const Play::Vector2D to_target = {aimTarget_->x - tank_->GetPosition().x, aimTarget_->y - tank_->GetPosition().y};
    const float desired_angle = Geom::angOf(to_target);
    const float current_angle = tank_->GetRotation();
    const float angle_diff = Geom::wrapAngle(desired_angle - current_angle);
    return std::fabs(angle_diff) <= profile_.ang_tol;
}

FollowCommand::Status MotionService::GetStatus() const {
    return lastStatus_;
}

} // namespace Motion
