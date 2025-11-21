#pragma once

/// @brief Decorator nodes (e.g., Repeat, Guards) for Behavior Tree.

#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Controllers/BT/BTConfig.h"
#include "AI/Gateway/AIServiceGateway.h"

namespace AI::BT {

// Repeat: keep child running indefinitely; restart child after it completes
class RepeatDecorator final : public Node {
public:
    explicit RepeatDecorator(std::unique_ptr<Node> child) : child_(std::move(child)) {}

    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override { if (child_) child_->OnEnter(bb, gw); }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override { if (child_) child_->OnExit(bb, gw); }

    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        if (!child_) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        Status s = child_->Tick(dt, bb, gw);
        if (s == Status::Running) { Debug_SetLastStatus_(Status::Running); return Status::Running; }
        // Completed; restart immediately
        child_->OnExit(bb, gw);
        child_->OnEnter(bb, gw);
        Debug_SetLastStatus_(Status::Running);
        return Status::Running;
    }

    [[nodiscard]] const char* DebugName() const override { return "Repeat"; }
    // Debug helper: expose inner node so overlays can descend
    [[nodiscard]] const Node* Debug_Inner() const { return child_.get(); }
private:
    std::unique_ptr<Node> child_{};
};

// GuardHighHP as a Composite with a single child: runs child only while hp > threshold
class GuardHighHP final : public Composite {
public:
    explicit GuardHighHP(const Config& cfg) : cfg_(cfg) {}

    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override {
        active_ = false;
        if (pred_(bb) && hasChild_()) { children_[0]->OnEnter(bb, gw); active_ = true; }
    }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override {
        if (active_ && hasChild_()) { children_[0]->OnExit(bb, gw); }
        active_ = false;
    }

    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        if (!hasChild_()) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        const bool ok = pred_(bb);
        if (!ok) {
            if (active_) { children_[0]->OnExit(bb, gw); active_ = false; }
            Debug_SetLastStatus_(Status::Failure);
            return Status::Failure;
        }
        if (!active_) { children_[0]->OnEnter(bb, gw); active_ = true; }
        const Status s = children_[0]->Tick(dt, bb, gw);
        Debug_SetLastStatus_(s);
        return s;
    }

    [[nodiscard]] const char* DebugName() const override { return "GuardHighHP"; }

private:
    [[nodiscard]] bool hasChild_() const { return !children_.empty(); }
    [[nodiscard]] bool pred_(const Blackboard& bb) const { return static_cast<int>(bb.self.hp) > cfg_.lowHpThreshold; }
    const Config& cfg_;
    bool active_{false};
};

// GuardLowHP as a Composite with a single child: runs child only while hp <= threshold
class GuardLowHP final : public Composite {
public:
    explicit GuardLowHP(const Config& cfg) : cfg_(cfg) {}

    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override {
        active_ = false;
        if (pred_(bb) && hasChild_()) { children_[0]->OnEnter(bb, gw); active_ = true; }
    }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override {
        if (active_ && hasChild_()) { children_[0]->OnExit(bb, gw); }
        active_ = false;
    }

    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        if (!hasChild_()) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        if (!pred_(bb)) {
            if (active_) { children_[0]->OnExit(bb, gw); active_ = false; }
            Debug_SetLastStatus_(Status::Failure);
            return Status::Failure;
        }
        if (!active_) { children_[0]->OnEnter(bb, gw); active_ = true; }
        const Status s = children_[0]->Tick(dt, bb, gw);
        Debug_SetLastStatus_(s);
        return s;
    }

    [[nodiscard]] const char* DebugName() const override { return "GuardLowHP"; }
private:
    [[nodiscard]] bool hasChild_() const { return !children_.empty(); }
    [[nodiscard]] bool pred_(const Blackboard& bb) const { return static_cast<int>(bb.self.hp) <= cfg_.lowHpThreshold; }
    const Config& cfg_;
    bool active_{false};
};

// GuardThreat as a Composite with a single child: runs child only while target is visible or lastKnown exists
class GuardThreat final : public Composite {
public:
    GuardThreat() = default;

    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override {
        active_ = false;
        if (pred_(bb, gw) && hasChild_()) { children_[0]->OnEnter(bb, gw); active_ = true; }
    }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override {
        if (active_ && hasChild_()) { children_[0]->OnExit(bb, gw); }
        active_ = false;
    }

    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        if (!hasChild_()) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        if (!pred_(bb, gw)) {
            if (active_) { children_[0]->OnExit(bb, gw); active_ = false; }
            Debug_SetLastStatus_(Status::Failure);
            return Status::Failure;
        }
        if (!active_) { children_[0]->OnEnter(bb, gw); active_ = true; }
        const Status s = children_[0]->Tick(dt, bb, gw);
        Debug_SetLastStatus_(s);
        return s;
    }

    [[nodiscard]] const char* DebugName() const override { return "GuardThreat"; }
private:
    [[nodiscard]] bool hasChild_() const { return !children_.empty(); }
    static bool pred_(const Blackboard& bb, AIServiceGateway& gw) {
        bool visible = false;
        if (bb.targetId) {
            const auto vis = gw.Sense_VisibleEnemies();
            for (const auto &c : vis) { if (c.id == *bb.targetId) { visible = true; break; } }
        }
        const bool hasLK = bb.lastKnown.has_value();
        return visible || hasLK;
    }
    bool active_{false};
};

} // namespace AI::BT
