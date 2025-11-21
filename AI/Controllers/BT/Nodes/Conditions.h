#pragma once

/// @brief Condition nodes for Behavior Tree (e.g., HP bracket checks).

#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Controllers/BT/BTConfig.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AI/Data/AIEvents.h"

namespace AI::BT {

class ConditionHighHP final : public Node {
public:
    explicit ConditionHighHP(const Config& cfg) : cfg_(cfg) {}
    Status Tick(float /*dt*/, Blackboard& bb, AIServiceGateway& /*gw*/) override {
        const bool ok = static_cast<int>(bb.self.hp) > cfg_.lowHpThreshold;
        Debug_SetLastStatus_(ok ? Status::Success : Status::Failure);
        return ok ? Status::Success : Status::Failure;
    }
    [[nodiscard]] const char* DebugName() const override { return "HighHP?"; }
private:
    const Config& cfg_;
};

class ConditionHasLastKnown final : public Node {
public:
    Status Tick(float /*dt*/, Blackboard& bb, AIServiceGateway& /*gw*/) override {
        const bool ok = bb.lastKnown.has_value();
        Debug_SetLastStatus_(ok ? Status::Success : Status::Failure);
        return ok ? Status::Success : Status::Failure;
    }
    [[nodiscard]] const char* DebugName() const override { return "HasLastKnown?"; }
};

class ConditionTargetVisible final : public Node {
public:
    Status Tick(float /*dt*/, Blackboard& bb, AIServiceGateway& gw) override {
        bool visible = false;
        if (bb.targetId) {
            const auto visibles = gw.Sense_VisibleEnemies();
            for (const auto &c : visibles) { if (c.id == *bb.targetId) { visible = true; break; } }
        }
        Debug_SetLastStatus_(visible ? Status::Success : Status::Failure);
        return visible ? Status::Success : Status::Failure;
    }
    [[nodiscard]] const char* DebugName() const override { return "TargetVisible?"; }
};

} // namespace AI::BT
