#include "AI/Controllers/DebugAIController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include <Play.h>

namespace AI {

void DebugAIController::Update(float deltaTime) {
  // Base hygiene: ensure event subscriptions are bound
  AIDecisionController::Update(deltaTime);

  if (!IsActive()) return;

  // Left click - cancel aim and move
  if (Play::MousePressed(MOUSE_BUTTON_LEFT)) {
    if (gateway_.IsAiming()) {
      gateway_.CancelAim();
      charging_ = false;
      firedThisAim_ = false;
    }
    const Play::Point2D m = Play::GetMousePos();
    gateway_.MoveTo({static_cast<float>(m.x), static_cast<float>(m.y)});
  }

  // Right-click toggles aiming at cursor
  if (Play::MousePressed(MOUSE_BUTTON_RIGHT)) {
    if (gateway_.IsAiming()) {

      gateway_.CancelAim();
      charging_ = false;
      firedThisAim_ = false;
    } else {
      const Play::Point2D m = Play::GetMousePos();
      gateway_.AimAt({static_cast<float>(m.x), static_cast<float>(m.y)});
    }
  }

  // Space: fire (hold to charge, release to fire)
  if (Play::KeyPressed(KEY_SPACE)) {
    gateway_.BeginFire();
    charging_ = true;
  }
  // Release when no longer held
  if (charging_ && !Play::KeyDown(KEY_SPACE)) {
    gateway_.ReleaseFire();
    charging_ = false;
  }

  // C: cancel move (and stop)
  if (Play::KeyPressed(KEY_C)) {
    gateway_.CancelMove();
  }

  // WASD: low-level driving
  int v = 0, w = 0;
  if (Play::KeyDown(KEY_W)) v += 1;
  if (Play::KeyDown(KEY_S)) v -= 1;
  if (Play::KeyDown(KEY_A)) w -= 1;
  if (Play::KeyDown(KEY_D)) w += 1;
  if (v != 0 || w != 0) {
    // Cancel any path-following to avoid conflicting commands
    gateway_.CancelMove();
    if (v != 0) gateway_.Drive(v);
    if (w != 0) gateway_.Turn(w);
  }

  // R: toggle random roam (repeat until canceled or R pressed again)
  if (Play::KeyPressed(KEY_R)) {
    roamEnabled_ = !roamEnabled_;
    if (roamEnabled_) {
      const auto target = gateway_.Nav_GetRandomReachable({});
      gateway_.MoveTo(target);
      firedThisAim_ = false;
    } else {
      gateway_.CancelMove();
    }
  }

  // If roaming and arrived/blocked, pick another
  if (roamEnabled_) {
    const auto st = gateway_.GetMotionStatus();
    if (st == Motion::FollowCommand::Status::Arrived ||
        st == Motion::FollowCommand::Status::Blocked) {
      const auto target = gateway_.Nav_GetRandomReachable({});
      gateway_.MoveTo(target);
    }
  }
}

}  // namespace AI
