#include "FSMDebugLayer.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Controllers/FSM/States/FSMStates.h"
#include "AISubsystem.h"
#include "Gateway/AIServiceGateway.h"
#include "Helper/Geometry.h"
#include "Play.h"
#include "Tank.h"

namespace AI {

namespace {
  Play::Colour StateColour(FSMState s) {
    switch (s) {
      case FSMState::Idle:      return Play::Colour{220,220,220,255};
      case FSMState::Patrol:    return Play::cWhite;
      case FSMState::LookAround:return Play::cCyan;
      case FSMState::Engage:    return Play::cRed;
      case FSMState::Search:    return Play::cYellow;
      case FSMState::Flee:      return Play::Colour{255,0,255,255};
    }
    return Play::cWhite;
  }
  const char* StateName(FSMState s) {
    switch (s) {
      case FSMState::Idle: return "Idle";
      case FSMState::Patrol: return "Patrol";
      case FSMState::LookAround: return "LookAround";
      case FSMState::Engage: return "Engage";
      case FSMState::Search: return "Search";
      case FSMState::Flee: return "Flee";
    }
    return "Unknown";
  }
}

void FSMDebugLayer::render(const AISubsystem& sys) const {
  if (!visible_) return;

  for (const auto &[fst, snd] : sys.getAgents()) {
    const auto id = fst;
    const auto &agent = snd;
    if (!agent.tank || !agent.controller) continue;

    const auto* fsm = dynamic_cast<const FSMController*>(agent.controller.get());
    if (!fsm) continue;

    const Play::Vector2D pos = agent.tank->GetPosition();
    const float R = agent.tank->GetRadius();

    const auto st = fsm->Debug_State();
    const auto col = StateColour(st);
    const char* stName = StateName(st);

    // Radii visualization for Engage only
    if (st == FSMState::Engage) {
      const auto& cfg = fsm->Config();
      Play::DrawCircle(pos, cfg.engageAttackRadius, Play::Colour{255,0,0,60});   // attack ring (red, translucent)
      Play::DrawCircle(pos, cfg.engageChaseRadius,  Play::Colour{255,255,0,40}); // chase ring (yellow, translucent)
    }

    char line1[128];
    if (st == FSMState::Engage) {
      float dist = -1.0f;
      if (auto tid = fsm->GetTargetId()) {
        for (const auto &c : agent.gw->Sense_VisibleEnemies()) { if (c.id == *tid) { dist = Geom::dist(agent.tank->GetPosition(), c.pos); break; } }
      }
      if (dist < 0.0f) { std::snprintf(line1, sizeof(line1), "T%u FSM: Engage", static_cast<unsigned>(id)); }
      else { std::snprintf(line1, sizeof(line1), "T%u FSM: Engage (d=%.0f)", static_cast<unsigned>(id), dist); }
    } else {
      std::snprintf(line1, sizeof(line1), "T%u FSM: %s", static_cast<unsigned>(id), stName);
    }

    char line2[48];
    std::snprintf(line2, sizeof(line2), "Target:-");

    // Draw above tank
    Play::DrawDebugText({ pos.x, pos.y + (R + 24.0f) }, line1, 12, col);
    Play::DrawDebugText({ pos.x, pos.y + (R + 10.0f) }, line2, 12, col);
  }
}

void FSMDebugLayer::handleInput(const AISubsystem& /*sys*/) {
  // No inputs yet
}

} // namespace AI
