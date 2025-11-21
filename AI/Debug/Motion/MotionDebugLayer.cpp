#include "AI/Debug/Motion/MotionDebugLayer.h"
#include "AI/AISubsystem.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "CoreTank/Tank.h"
#include "Globals.h"
#include "Motion/MotionService.h"
#include <Play.h>
#include <cmath>

namespace AI {

namespace {
  Play::Vector2D UnitFromAngle(float a) { return { std::cos(a), std::sin(a) }; }
}

void MotionDebugLayer::render(const AISubsystem& sys) const {
  if (!visible_) return;

  for (const auto& [id, agent] : sys.getAgents()) {
    if (!agent.tank || !agent.gw) continue;

    const auto &tank = *agent.tank;
    const auto &gw   = *agent.gw;

    const float R = tank.GetRadius();
    const Play::Vector2D pos = tank.GetPosition();

    // Body heading line
    const Play::Vector2D head = { pos.x + UnitFromAngle(tank.GetRotation()).x * R,
                                  pos.y + UnitFromAngle(tank.GetRotation()).y * R };
    Play::DrawLine(pos, head, Play::cWhite);

    // Motion status label
    const auto status = gw.GetMotionStatus();
    const char* statusStr = "Idle";
    Play::Colour col = Play::cWhite;
    switch (status) {
      case Motion::FollowCommand::Status::Following: statusStr = "Following"; col = Play::cBlue; break;
      case Motion::FollowCommand::Status::Arrived:   statusStr = "Arrived";   col = Play::cGreen; break;
      case Motion::FollowCommand::Status::Blocked:   statusStr = "Blocked";   col = Play::cRed;   break;
      default: break;
    }

    char line[64];
    std::snprintf(line, sizeof(line), "T%u: %s", static_cast<unsigned>(id), statusStr);
    Play::DrawDebugText({ pos.x, pos.y - (R + 12.0f) }, line, 12, col);

    // Lookahead point and goal marker
    if (agent.motion) {
      if (const auto la = agent.motion->Debug_GetLookahead()) {
        Play::DrawCircle(*la, 3.0f, Play::cCyan);
      }
      if (const auto goal = agent.motion->Debug_GetGoal()) {
        Play::DrawCircle(*goal, 5.f, Play::cGreen);
        Play::DrawLine(pos, *goal, Play::Colour{0,255,0,140});
      }
    }
  }
}

} // namespace AI
