#include "AI/Debug/BT/BTDebugLayer.h"
#include "AI/AISubsystem.h"
#include "AI/Controllers/BT/BehaviorTreeController.h"
#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "CoreTank/Tank.h"
#include <Play.h>
#include <cstdio>

namespace AI {

namespace {
  // Best effort to get a child label for composites
  const char* ChildName(const BT::Node* n) {
    return n ? n->DebugName() : "?";
  }
}

void BTDebugLayer::render(const AISubsystem& sys) const {
  if (!visible_) return;

  for (const auto& [id, agent] : sys.getAgents()) {
    if (!agent.tank || !agent.controller) continue;
    const auto* bt = dynamic_cast<const BT::BehaviorTreeController*>(agent.controller.get());
    if (!bt) continue; // other controller types ignored

    const Play::Vector2D pos = agent.tank->GetPosition();
    const float R = agent.tank->GetRadius();

    const bool active = bt->IsActive();
    const char* rootLabel = active ? bt->Debug_GetRootName() : "Inactive";

    // Derive HP bracket
    const int hp = static_cast<int>(agent.tank->GetHealth());
    const char* hpBracket = (hp <= 1) ? "LowHP" : "HighHP";

    // Compute leaf name
    std::string leafName = rootLabel;
    if (active) {
        leafName = bt->Debug_CurrentLeafName();
    }

    char line[160];
    std::snprintf(line, sizeof(line), "T%u BT: %s [%s]", static_cast<unsigned>(id), leafName.c_str(), hpBracket);
    Play::DrawDebugText({ pos.x, pos.y + (R + 24.0f) }, line, 12, Play::cWhite);
  }
}

void BTDebugLayer::handleInput(const AISubsystem& /*sys*/) {
  // No inputs for BT layer beyond visibility toggle handled by overlay.
}

} // namespace AI
