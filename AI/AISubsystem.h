#pragma once

/// @brief Orchestrates per-tank AI services (pathfinding, motion, sensing, combat) and controllers; owns shared audio bus and updates each frame.

#include <memory>
#include <unordered_map>

class Tank;

namespace Pathfinding { class PathfinderService; }
namespace Motion      { class MotionService;     }
namespace Combat      { class CombatService;     }
namespace Sensing     { class SensingService;    }
namespace Sensing::Audio { class Bus; }

namespace AI {
class AIServiceGateway;
class AIDecisionController;
class AIDebugOverlay;

using TankId = std::uint32_t;

struct AgentCtx {
    Tank* tank = nullptr;

    // Per-tank services
    std::unique_ptr<Motion::MotionService>   motion;
    std::unique_ptr<Combat::CombatService>   combat;
    std::unique_ptr<Sensing::SensingService> sensing;

    // Facade seen by controllers
    std::unique_ptr<AIServiceGateway>        gw;

    // Decision controller
    std::unique_ptr<AIDecisionController>    controller;

    // Alive mirror for respawn detection and state reset
    bool wasAlive{false};
};

class AISubsystem {
public:
    AISubsystem();
    ~AISubsystem();

    // Per-tank lifecycle
    void addTank(TankId id, Tank* tank, std::unique_ptr<AIDecisionController> controller);
    void removeTank(TankId id);
    void setController(TankId id, std::unique_ptr<AIDecisionController> controller);

    // Controller API access
    AIServiceGateway& gateway(TankId id);

    // Per-frame update
    void tick(float dt);

    void renderDebugOverlay();

    // Shared services access
    Pathfinding::PathfinderService& pathfinder() const;

    // Expose agents for debug purposes
    const std::unordered_map<TankId, AgentCtx>& getAgents() const { return agents_; }

    // Debug-only: audio bus exposure for overlays
    const Sensing::Audio::Bus* Debug_GetAudioBus() const { return audioBus_.get(); }

    // Global activation for AI controllers
    void SetAIEnabled(bool on);
    [[nodiscard]] bool GetAIEnabled() const { return aiEnabled_; }

private:
    // Shared
    std::unique_ptr<Pathfinding::PathfinderService> pathfinding_;
    std::unique_ptr<Sensing::Audio::Bus>            audioBus_;

    // All agents by TankId
    std::unordered_map<TankId, AgentCtx> agents_;

    bool aiEnabled_{false};

#ifdef AI_DEBUG
    std::unique_ptr<AIDebugOverlay> debugOverlay_;
#endif

public:
    // No copying
    AISubsystem(const AISubsystem&) = delete;
    AISubsystem& operator=(const AISubsystem&) = delete;
};

} // namespace AI