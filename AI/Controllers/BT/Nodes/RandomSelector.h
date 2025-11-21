#pragma once

/// @brief Composite that picks one random child, runs it to completion, then picks again.

#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Controllers/BT/Util/Random.h"

namespace AI::BT {

class RandomSelector final : public Composite {
public:
    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override {
        if (children_.empty()) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }
        if (!active_) selectNew_(bb, gw);
        auto& child = children_[current_];
        const Status s = child->Tick(dt, bb, gw);
        if (s == Status::Running) { Debug_SetLastStatus_(Status::Running); return Status::Running; }
        // Child finished: restart with a new random choice
        child->OnExit(bb, gw);
        selectNew_(bb, gw);
        Debug_SetLastStatus_(Status::Running);
        return Status::Running;
    }
    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override { active_ = false; selectNew_(bb, gw); }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override {
        if (active_ && current_ < children_.size()) children_[current_]->OnExit(bb, gw);
        active_ = false;
    }
    [[nodiscard]] const char* DebugName() const override { return "RandSel"; }
    [[nodiscard]] std::optional<std::size_t> Debug_CurrentIndex() const { return active_ ? std::optional<std::size_t>(current_) : std::nullopt; }
private:
    std::size_t current_{0};
    bool active_{false};
    void selectNew_(Blackboard& bb, AIServiceGateway& gw) {
        std::uniform_int_distribution<std::size_t> dist(0, children_.size() - 1);
        current_ = dist(Rand::Engine());
        active_ = true;
        children_[current_]->OnEnter(bb, gw);
    }
};

} // namespace AI::BT
