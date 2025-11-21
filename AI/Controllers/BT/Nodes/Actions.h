#pragma once

/// @brief Action nodes for patrol movement, look-around scanning, search, engage, flee, and idle fallback.

#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Controllers/BT/BTConfig.h"
#include "AI/Controllers/BT/Util/Random.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AI/Controllers/BT/Blackboard.h"
#include "Helper/Geometry.h"
#include "AI/Behaviors/CommonBehaviors.h"

namespace AI::BT {

// Patrol: move to a random reachable point; succeed when arrived or blocked
class PatrolAction final : public Node {
public:
    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override {

        goal_ = gw.Nav_GetRandomReachable({});
        gw.MoveTo(goal_);
        active_ = true;
    }
    Status Tick(float /*dt*/, Blackboard& /*bb*/, AIServiceGateway& gw) override {
        if (!active_) { Debug_SetLastStatus_(Status::Success); return Status::Success; }
        const auto st = gw.GetMotionStatus();
        if (st == Motion::FollowCommand::Status::Following) { Debug_SetLastStatus_(Status::Running); return Status::Running; }
        if (st == Motion::FollowCommand::Status::Arrived || st == Motion::FollowCommand::Status::Blocked || st == Motion::FollowCommand::Status::Idle) {
            active_ = false; Debug_SetLastStatus_(Status::Success); return Status::Success; }
        Debug_SetLastStatus_(Status::Running); return Status::Running;
    }
    void OnExit(Blackboard& /*bb*/, AIServiceGateway& gw) override { if (active_) { gw.CancelMove(); active_ = false; } }
    [[nodiscard]] const char* DebugName() const override { return "Patrol"; }
private:
    bool active_{false};
    Play::Vector2D goal_{};
};

// LookAround phases: idle -> rotate A -> pause -> rotate B -> success
class LookAroundAction final : public Node {
public:
    explicit LookAroundAction(const Config& cfg) : cfg_(cfg) {}

    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override {
        gw.CancelMove();
        const bool swap = Rand::Chance(cfg_.look_swap_dir_chance);
        firstDir_ = swap ? 1 : -1;
        secondDir_ = -firstDir_;
        phase_ = Phase::Idle1;
        bb.lookPhase = 0;
        timer_ = 0.0f;
        phaseDur_ = Rand::Float(cfg_.look_idle_min, cfg_.look_idle_max);
        gw.Turn(0);
    }

    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        timer_ += dt;
        switch (phase_) {
            case Phase::Idle1:
                gw.Turn(0);
                if (timer_ >= phaseDur_) advance_(Phase::RotateA, bb, gw, Rand::Float(cfg_.look_rotate_min, cfg_.look_rotate_max));
                break;
            case Phase::RotateA:
                gw.Turn(firstDir_);
                if (timer_ >= phaseDur_) advance_(Phase::Pause, bb, gw, Rand::Float(cfg_.look_pause_min, cfg_.look_pause_max));
                break;
            case Phase::Pause:
                gw.Turn(0);
                if (timer_ >= phaseDur_) advance_(Phase::RotateB, bb, gw, Rand::Float(cfg_.look_rotate_min, cfg_.look_rotate_max));
                break;
            case Phase::RotateB:
                gw.Turn(secondDir_);
                if (timer_ >= phaseDur_) advance_(Phase::Done, bb, gw, 0.0f);
                break;
            case Phase::Done:
                Debug_SetLastStatus_(Status::Success);
                return Status::Success;
        }
        Debug_SetLastStatus_(Status::Running);
        return Status::Running;
    }

    void OnExit(Blackboard& /*bb*/, AIServiceGateway& gw) override { gw.Turn(0); }
    [[nodiscard]] const char* DebugName() const override { return "LookAround"; }

private:
    enum class Phase { Idle1, RotateA, Pause, RotateB, Done };
    const Config& cfg_;
    Phase phase_{Phase::Idle1};
    float timer_{0.0f};
    float phaseDur_{0.0f};
    int firstDir_{-1};
    int secondDir_{1};

    void advance_(Phase next, Blackboard& bb, AIServiceGateway& gw, float nextDur) {
        phase_ = next;
        bb.lookPhase = static_cast<int>(next);
        timer_ = 0.0f;
        phaseDur_ = nextDur;
        if (next == Phase::Done) gw.Turn(0);
    }
};

// Simple idle fallback
class IdleForever final : public Node {
public:
    Status Tick(float /*dt*/, Blackboard& /*bb*/, AIServiceGateway& /*gw*/) override { Debug_SetLastStatus_(Status::Running); return Status::Running; }
    [[nodiscard]] const char* DebugName() const override { return "Idle"; }
};

// Search/investigate: move to lastKnown position until arrival or memory expires.
// NOTE: On Arrived/Blocked, intentionally return Failure to let the selector fall through
// to patrol/look on the same tick.
// TODO: fix the syntax to handle success properly in the BT structure
class SearchAction final : public Node {
public:
    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override {
        active_ = false;
        goal_.reset();
        if (bb.targetId) {
            if (auto lk = gw.Sense_LastKnown(*bb.targetId)) { goal_ = lk->pos; }
        }
        if (!goal_ && bb.lastKnown) goal_ = bb.lastKnown;
        if (goal_) { gw.MoveTo(*goal_); active_ = true; }
    }
    Status Tick(float /*dt*/, Blackboard& bb, AIServiceGateway& gw) override {
        if (!goal_) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }

        // Target memory expired mid-investigation -> fail so selector can fall through this tick
        if (bb.targetId && !gw.Sense_LastKnown(*bb.targetId).has_value()) {
            bb.lastKnown.reset(); bb.targetId.reset(); Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        const auto st = gw.GetMotionStatus();
        if (st == Motion::FollowCommand::Status::Arrived) {
            bb.lastKnown.reset(); Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        if (st == Motion::FollowCommand::Status::Blocked) {
            bb.lastKnown.reset(); Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        Debug_SetLastStatus_(Status::Running); return Status::Running;
    }
    void OnExit(Blackboard& /*bb*/, AIServiceGateway& gw) override { if (active_) { gw.CancelMove(); active_ = false; } }
    [[nodiscard]] const char* DebugName() const override { return "Search"; }
private:
    bool active_{false};
    std::optional<Play::Vector2D> goal_{};
};

// Engage: aim and fire or chase target (refined with charge & LOS + grace timer)
class EngageAction final : public Node {
public:
    explicit EngageAction(const Config& cfg) : cfg_(cfg) {}
    void OnEnter(Blackboard& /*bb*/, AIServiceGateway& gw) override {
        chasing_ = false; aiming_ = false; firing_ = false; rearming_ = false;
        fireCooldown_ = 0.0f; lostVisTimer_ = 0.0f;
        gw.BeginFire(); firing_ = true; // start charging immediately
    }
    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        if (!bb.targetId) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }

        bool visible = false; Play::Vector2D targetPos{}; bool havePos = false;
        if (bb.targetId) {
            const auto visibles = gw.Sense_VisibleEnemies();
            for (const auto &c : visibles) { if (c.id == *bb.targetId) { visible = true; targetPos = c.pos; havePos = true; break; } }
        }
        if (!visible) {
            if (auto lk = gw.Sense_LastKnown(*bb.targetId)) { targetPos = lk->pos; havePos = true; }
            lostVisTimer_ += dt; if (lostVisTimer_ >= 0.30f) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        } else { lostVisTimer_ = 0.0f; }
        if (!havePos) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }

        const float d = Geom::dist(bb.self.pos, targetPos);
        const bool inAttack = d <= cfg_.engageAttackRadius;
        fireCooldown_ = std::max(0.0f, fireCooldown_ - dt);

        if (!inAttack) {
            if (aiming_) { gw.CancelAim(); aiming_ = false; }
            gw.MoveTo(targetPos); chasing_ = true;
        } else {
            if (chasing_) { gw.CancelMove(); chasing_ = false; }

            // Continuous re-aim
            if (!aiming_ || !gw.IsOnTarget() || visible) { gw.AimAt(targetPos); aiming_ = true; }

            const float charge = gw.Combat_ChargeAccum();
            const float needed = 1.0f; // fully charge before firing
            const bool chargedEnough = charge >= needed;
            const bool hasLOS = gw.Sense_HasLOS(bb.self.pos, targetPos);

            if (chargedEnough && hasLOS && gw.IsOnTarget() && fireCooldown_ <= 0.0f && !rearming_) {
                gw.ReleaseFire();
                rearming_ = true; rearmTimer_ = 0.05f; fireCooldown_ = cfg_.fireCadenceSec;
            }
        }

        if (rearming_) {
            rearmTimer_ -= dt;
            if (rearmTimer_ <= 0.0f) { gw.BeginFire(); rearming_ = false; firing_ = true; }
        }

        Debug_SetLastStatus_(Status::Running);
        return Status::Running;
    }
    void OnExit(Blackboard& /*bb*/, AIServiceGateway& gw) override {
        if (chasing_) gw.CancelMove();
        if (aiming_) gw.CancelAim();
        if (gw.Combat_IsCharging()) gw.ReleaseFire();
        chasing_ = false; aiming_ = false; firing_ = false; rearming_ = false;
    }
    [[nodiscard]] const char* DebugName() const override { return "Engage"; }
private:
    const Config& cfg_;
    bool chasing_{false}; bool aiming_{false}; bool firing_{false}; bool rearming_{false};
    float rearmTimer_{0.0f}; float fireCooldown_{0.0f}; float lostVisTimer_{0.0f};
};

// MicroPatrol: cautious movement for low HP; succeed after duration or arrival/blocked
class MicroPatrolAction final : public Node {
public:
    explicit MicroPatrolAction(const Config& cfg) : cfg_(cfg) {}
    void OnEnter(Blackboard& /*bb*/, AIServiceGateway& gw) override {
        timer_ = 0.0f;
        gw.MoveTo(gw.Nav_GetRandomReachable({}));
        active_ = true;
    }
    Status Tick(float dt, Blackboard& /*bb*/, AIServiceGateway& gw) override {
        if (!active_) { Debug_SetLastStatus_(Status::Success); return Status::Success; }
        timer_ += dt;
        const auto st = gw.GetMotionStatus();
        if (st == Motion::FollowCommand::Status::Arrived || st == Motion::FollowCommand::Status::Blocked) { active_ = false; Debug_SetLastStatus_(Status::Success); return Status::Success; }
        if (timer_ >= cfg_.lowHpMicroPatrolSec) { active_ = false; Debug_SetLastStatus_(Status::Success); return Status::Success; }
        Debug_SetLastStatus_(Status::Running); return Status::Running;
    }
    void OnExit(Blackboard& /*bb*/, AIServiceGateway& gw) override { if (active_) { gw.CancelMove(); active_ = false; } }
    [[nodiscard]] const char* DebugName() const override { return "MicroPatrol"; }
private:
    const Config& cfg_;
    float timer_{0.0f};
    bool active_{false};
};

// Flee: escape to a safe location away from threat (visible target OR lastKnown).
class FleeAction final : public Node {
public:
    Status Tick(float /*dt*/, Blackboard& bb, AIServiceGateway& gw) override {
        if (!active_) {
            gw.ReleaseFire(); gw.CancelAim();
            threatPos_ = resolveThreat_(bb, gw);
            if (!threatPos_) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
            fleePoint_ = AI::Behaviors::FindFleeLocation(bb.self.pos, threatPos_, gw);
            if (Geom::dist(fleePoint_, bb.self.pos) <= 1.0f) { bb.lastKnown.reset(); Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
            gw.MoveTo(fleePoint_); active_ = true; Debug_SetLastStatus_(Status::Running); return Status::Running;
        }
        const auto st = gw.GetMotionStatus();
        if (st == Motion::FollowCommand::Status::Following) { Debug_SetLastStatus_(Status::Running); return Status::Running; }
        if (st == Motion::FollowCommand::Status::Arrived || st == Motion::FollowCommand::Status::Blocked || st == Motion::FollowCommand::Status::Idle) {
            active_ = false; bb.lastKnown.reset(); Debug_SetLastStatus_(Status::Success); return Status::Success; }
        Debug_SetLastStatus_(Status::Running); return Status::Running;
    }
    void OnExit(Blackboard& /*bb*/, AIServiceGateway& gw) override { if (active_) { gw.CancelMove(); active_ = false; } }
    [[nodiscard]] const char* DebugName() const override { return "Flee"; }
private:
    bool active_{false};
    std::optional<Play::Vector2D> threatPos_{};
    Play::Vector2D fleePoint_{};
    static std::optional<Play::Vector2D> resolveThreat_(Blackboard& bb, AIServiceGateway& gw) {
        if (bb.targetId) {
            const auto vis = gw.Sense_VisibleEnemies();
            for (const auto &c : vis) { if (c.id == *bb.targetId) return c.pos; }
        }
        if (bb.lastKnown) return bb.lastKnown; return std::nullopt; }
};

} // namespace AI::BT
