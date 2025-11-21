#include "AIDebugOverlay.h"
#include <Play.h>

#include "AISubsystem.h"
#include "Globals.h"
#include "Pathfinding/PathfindingDebugLayer.h"
#include "Motion/MotionDebugLayer.h"
#include "Sensing/SensingDebugLayer.h"
#include "FSM/FSMDebugLayer.h"
#include "BT/BTDebugLayer.h"

namespace AI {

AIDebugOverlay::AIDebugOverlay() {
  pathfindingLayer_ = std::make_unique<PathfindingDebugLayer>();
  motionLayer_ = std::make_unique<MotionDebugLayer>();
  sensingLayer_ = std::make_unique<SensingDebugLayer>();
  fsmLayer_ = std::make_unique<FSMDebugLayer>();
  btLayer_  = std::make_unique<BTDebugLayer>();
}

AIDebugOverlay::~AIDebugOverlay() = default;

void AIDebugOverlay::render(const AISubsystem& sys) const {

  // Left band: main HUD (avoid clipping, avoid arena)
  const int leftX = 120;
  int y = DISPLAY_HEIGHT - 24;
  Play::DrawDebugText({ leftX, y }, "AI Debug Overlay Active", 16, Play::cWhite);
  y -= 20;
  const bool aiOn = sys.GetAIEnabled();
  Play::DrawDebugText({ leftX, y }, aiOn ? "AI: ON (Enter to toggle)" : "AI: OFF (Enter to toggle)", 14, aiOn ? Play::cGreen : Play::cRed);
  y -= 18;
  Play::DrawDebugText({ leftX + 20, y }, "F1 Pathfinding | F2 Motion | F3 Sensing | F4 FSM | F5 BT", 10, Play::cWhite);

  // Right band: only show active layers
  const int rightX = DISPLAY_WIDTH - 100; // safe margin inside the band
  int ry = DISPLAY_HEIGHT - 24;
  if (pathfindingLayer_->isVisible()) { Play::DrawDebugText({ rightX, ry }, "Pathfinding", 15, Play::cWhite); ry -= 18; }
  if (motionLayer_->isVisible())      { Play::DrawDebugText({ rightX, ry }, "Motion", 15, Play::cWhite);        ry -= 18; }
  if (sensingLayer_->isVisible())     { Play::DrawDebugText({ rightX, ry }, "Sensing", 15, Play::cWhite);       ry -= 18; }
  if (fsmLayer_->isVisible())         { Play::DrawDebugText({ rightX, ry }, "FSM", 15, Play::cWhite);           ry -= 18; }
  if (btLayer_->isVisible())          { Play::DrawDebugText({ rightX, ry }, "BT", 15, Play::cWhite), ry -= 18; }

  if (pathfindingLayer_->isVisible()) pathfindingLayer_->render(sys);
  if (motionLayer_->isVisible())      motionLayer_->render(sys);
  if (sensingLayer_->isVisible())     sensingLayer_->render(sys);
  if (fsmLayer_->isVisible())         fsmLayer_->render(sys);
  if (btLayer_->isVisible())          btLayer_->render(sys);
}


void AIDebugOverlay::handleInput(AISubsystem &sys) const {

  if (Play::KeyPressed(KEY_F1)) {
    pathfindingLayer_->toggleVisibility();
  }
  if (Play::KeyPressed(KEY_F2)) {
    motionLayer_->toggleVisibility();
  }
  if (Play::KeyPressed(KEY_F3)) {
    sensingLayer_->toggleVisibility();
  }
  if (Play::KeyPressed(KEY_F4)) {
    fsmLayer_->toggleVisibility();
  }
  if (Play::KeyPressed(KEY_F5)) {
    btLayer_->toggleVisibility();
  }

  // Global AI toggle on Enter
  if (Play::KeyPressed(KEY_ENTER)) {
    sys.SetAIEnabled(!sys.GetAIEnabled());
  }

  if (pathfindingLayer_->isVisible()) {
    pathfindingLayer_->handleInput(sys);
  }
  if (motionLayer_->isVisible()) {
    motionLayer_->handleInput(sys);
  }
  if (sensingLayer_->isVisible()) {
    sensingLayer_->handleInput(sys);
  }
  if (fsmLayer_->isVisible()) {
    fsmLayer_->handleInput(sys);
  }
  if (btLayer_->isVisible()) {
    btLayer_->handleInput(sys);
  }
}

} // namespace AI
