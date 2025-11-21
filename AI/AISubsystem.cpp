#include "AISubsystem.h"

#include "AI/Controllers/AIDecisionController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "Tank.h"

#include "Services/Pathfinding/Pathfinding.h"
#include "Services/Motion/Motion.h"
#include "Services/Combat/Combat.h"
#include "Services/Sensing/Sensing.h"
#include "Services/Sensing/Audio/Bus.h"

#include "AI/Data/AIContext.h"

#include <ranges>

#ifdef AI_DEBUG
#include "Debug/AIDebugOverlay.h"
#endif

namespace AI {

AISubsystem::AISubsystem()
{
    pathfinding_ = std::make_unique<Pathfinding::PathfinderService>();
    audioBus_    = std::make_unique<Sensing::Audio::Bus>();
    // Params setting and graph building is done in MainGame.cpp after structures are initialized.

#ifdef AI_DEBUG
    debugOverlay_ = std::make_unique<AIDebugOverlay>();
#endif
}

AISubsystem::~AISubsystem() = default;

void AISubsystem::addTank(TankId id, Tank* tank, std::unique_ptr<AIDecisionController> controller)
{
    AgentCtx ctx;
    ctx.tank    = tank;
    ctx.motion  = std::make_unique<Motion::MotionService>();
    ctx.combat  = std::make_unique<Combat::CombatService>();
    ctx.sensing = std::make_unique<Sensing::SensingService>();

    // Bind per-tank services to the Tank*
    ctx.motion->BindTank(tank);
    ctx.combat->BindTank(tank);
    // Sensing has no BindTank; it gets SelfState every frame.

    const bool emit = (controller != nullptr);
    // Create gateway exposing only high-level API to controllers
    ctx.gw = std::make_unique<AIServiceGateway>(
        *pathfinding_, ctx.motion.get(), ctx.sensing.get(), ctx.combat.get(), audioBus_.get(), emit
    );


    // Install controller
    ctx.controller = std::move(controller);

    // Subscribe to tank lifecycle events to manage controller state
    if (tank)
    {
        tank->SetOnDeath([this, id](){
            auto& agentCtx = agents_.at(id);
            if (agentCtx.controller) agentCtx.controller->SetActive(false);

            // Notify all other agents to forget this tank
            for (auto& [otherId, otherAgentCtx] : agents_)
            {
                if (otherId == id) continue;
                if (otherAgentCtx.sensing) otherAgentCtx.sensing->ForgetTarget(id);
            }
        });
        tank->SetOnRespawn([this, id](){
            auto& agentCtx = agents_.at(id);
            if (agentCtx.controller) agentCtx.controller->SetActive(true);
        });
        tank->SetOnDamage([this, id](int amount){
            auto& agentCtx = agents_.at(id);
            if (agentCtx.gw) agentCtx.gw->NotifyDamageTaken(amount);
        });
    }

    // Initialize alive mirror
    ctx.wasAlive = (tank != nullptr) ? (tank->GetHealth() > 0) : false;

    agents_.emplace(id, std::move(ctx));
}

void AISubsystem::removeTank(const TankId id)
{
    agents_.erase(id);
}

void AISubsystem::setController(const TankId id, std::unique_ptr<AIDecisionController> controller) {
    auto &ctx = agents_.at(id);
    ctx.controller = std::move(controller);
    // Toggle sound emission only for AI-controlled agents because it's simpler for now
    if (ctx.gw) ctx.gw->SetSoundEmissionEnabled(ctx.controller != nullptr);
}

AIServiceGateway& AISubsystem::gateway(const TankId id)
{
    return *agents_.at(id).gw;
}

void AISubsystem::tick(const float dt)
{
    // Decay global audio bus
    if (audioBus_) audioBus_->Decay(dt);

    for (auto &a : agents_ | std::views::values) {
        // Build/update SelfState and run sensing
        SelfState self{};
        self.pos    = a.tank->GetPosition();
        self.rot    = a.tank->GetRotation();
        self.radius = a.tank->GetRadius();
        self.hp     = a.tank->GetHealth();
        self.id     = static_cast<std::uint32_t>(a.tank->GetID());

        // Respawn hygiene: detect false -> true and clear per-agent transient state
        const bool isAlive = (self.hp > 0);
        if (!a.wasAlive && isAlive) {
            if (a.gw) a.gw->Reset();
            // Optional: a.sensing->ClearMemory(); TTL currently handles this;
        }
        a.wasAlive = isAlive;

        a.gw->SetSelfState(self);
        a.gw->TickSensing(dt);

        // 2. Let controllers 'think'
        if (a.controller) a.controller->Update(dt);

        // 3. Execute motion/combat for the frame
        a.motion->Tick(dt, self);
        a.combat->Tick(dt, self);
    }

#ifdef AI_DEBUG
    if (debugOverlay_) {
        debugOverlay_->handleInput(*this);
    }
#endif
}

void AISubsystem::renderDebugOverlay() {
#ifdef AI_DEBUG
    if (debugOverlay_) {
        debugOverlay_->render(*this);
    }
#endif
}

Pathfinding::PathfinderService& AISubsystem::pathfinder() const {
    return *pathfinding_;
}

void AISubsystem::SetAIEnabled(const bool on) {
    aiEnabled_ = on;
    for (auto &val : agents_ | std::views::values) {
        auto &ctx = val;
        if (ctx.controller) {
            ctx.controller->SetActive(on);
        }
    }
}

} // namespace AI
