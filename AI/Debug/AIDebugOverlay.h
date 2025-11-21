#pragma once

/// @brief Provides a debug overlay for AI subsystems, allowing visualization of pathfinding, motion, sensing, and FSM data.

#include <memory>

namespace AI {

class AISubsystem;
class PathfindingDebugLayer;
class MotionDebugLayer;
class SensingDebugLayer;
class FSMDebugLayer;
class BTDebugLayer;

class AIDebugOverlay {
public:
    AIDebugOverlay();
    ~AIDebugOverlay();

    void render(const AISubsystem& sys) const;
    void handleInput(AISubsystem& sys) const;

private:
    std::unique_ptr<PathfindingDebugLayer> pathfindingLayer_;
    std::unique_ptr<MotionDebugLayer>      motionLayer_;
    std::unique_ptr<SensingDebugLayer>     sensingLayer_;
    std::unique_ptr<FSMDebugLayer>         fsmLayer_;
    std::unique_ptr<BTDebugLayer>          btLayer_;
};

} // namespace AI
