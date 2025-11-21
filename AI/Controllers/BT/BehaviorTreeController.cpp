#include "BehaviorTreeController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AI/Controllers/BT/Nodes/RandomSelector.h"
#include "AI/Controllers/BT/Nodes/Decorators.h"

namespace AI::BT {

namespace {
    std::optional<std::size_t> CurrentChildIndex(const Node* n) {
        if (!n) return std::nullopt;
        if (auto rep = dynamic_cast<const RepeatDecorator*>(n)) {
            // Always descend into inner
            return {0};
        }
        if (const auto sel = dynamic_cast<const Selector*>(n)) return sel->Debug_CurrentIndex();
        if (const auto seq = dynamic_cast<const Sequence*>(n)) return seq->Debug_CurrentIndex();
        if (const auto rnd = dynamic_cast<const RandomSelector*>(n)) return rnd->Debug_CurrentIndex();
        if (const auto guardH = dynamic_cast<const GuardHighHP*>(n)) return guardH->Debug_ChildCount() ? std::optional<std::size_t>(0) : std::nullopt;
        if (const auto guardL = dynamic_cast<const GuardLowHP*>(n)) return guardL->Debug_ChildCount() ? std::optional<std::size_t>(0) : std::nullopt;
        if (const auto threat = dynamic_cast<const GuardThreat*>(n)) return threat->Debug_ChildCount() ? std::optional<std::size_t>(0) : std::nullopt;
        return std::nullopt;
    }
}

void BehaviorTreeController::SetActive(const bool on) {
    AIDecisionController::SetActive(on);
    if (on) {
        if (!root_) buildTree_();
        if (root_) root_->OnEnter(bb_, gateway_);
    } else {
        if (root_) root_->OnExit(bb_, gateway_);
        bb_.ClearTransient();
    }
}

void BehaviorTreeController::onSpotted_(const SpottedEvent& e) {
    AIDecisionController::onSpotted_(e);
    bb_.targetId = e.id;
    if (auto lk = gateway_.Sense_LastKnown(e.id)) {
        bb_.lastKnown = lk->pos;
    }
}

void BehaviorTreeController::onLostSight_(const LostSightEvent& e) {
    AIDecisionController::onLostSight_(e);
    if (auto lk = gateway_.Sense_LastKnown(e.id)) {
        bb_.lastKnown = lk->pos;
    } else {
        bb_.targetId.reset();
        bb_.lastKnown.reset();
    }
}

void BehaviorTreeController::onSound_(const SoundHeardEvent& e) {
    AIDecisionController::onSound_(e);
    bb_.lastKnown = e.center;
    bb_.soundTimer = 0.0f;
}

void BehaviorTreeController::onDamage_(const int amount) {
    AIDecisionController::onDamage_(amount);
    bb_.damagedTimer = 0.0f;

    // Force re-evaluation: exit current tree and re-enter to restart from root
    if (root_) {
        root_->OnExit(bb_, gateway_);
        root_->OnEnter(bb_, gateway_);
    }
}

void BehaviorTreeController::buildTree_() {
    root_ = builder_ ? builder_() : BuildSimpleCombatTree(cfg_);
}

void BehaviorTreeController::Update(const float dt) {
    AIDecisionController::Update(dt);
    if (!IsActive()) return;

    // Mirror self before status detection
    const auto selfState = gateway_.Self();
    const bool aliveNow = selfState.hp > 0;

    // Detect respawn (was dead -> alive)
    if (!wasAlive_ && aliveNow) {
        bb_.ClearAll();
        if (!root_) buildTree_();
        if (root_) { root_->OnExit(bb_, gateway_); root_->OnEnter(bb_, gateway_); }
    }
    // Detect death (was alive -> dead)
    if (wasAlive_ && !aliveNow) {
        bb_.ClearAll();
        if (root_) root_->OnExit(bb_, gateway_);
    }
    wasAlive_ = aliveNow;

    bb_.self = selfState;
    bb_.soundTimer += dt;
    bb_.damagedTimer += dt;

    if (!root_) buildTree_();
    if (aliveNow && root_) (void)root_->Tick(dt, bb_, gateway_);
}

std::vector<std::uint32_t> BehaviorTreeController::Debug_CurrentPathIds() const {
    std::vector<std::uint32_t> path; if (!root_) return path; const Node* cursor = root_.get();
    while (cursor) {
        path.push_back(cursor->Debug_Id());

        // Repeat transparency
        if (const auto rep = dynamic_cast<const RepeatDecorator*>(cursor)) {
            const Node* inner = rep->Debug_Inner();
            if (!inner) break; cursor = inner; continue; }

        const auto* comp = dynamic_cast<const Composite*>(cursor);
        if (!comp) break;
        const auto idx = CurrentChildIndex(cursor);
        if (!idx) break;
        const Node* child = comp->Debug_Child(*idx);
        if (!child) break;
        cursor = child;
    }
    return path;
}

std::string BehaviorTreeController::Debug_CurrentLeafName() const {
    if (!root_) return "NoTree";
    const Node* cursor = root_.get(); const Node* last = cursor;
    while (cursor) {
        last = cursor;

        // Special handling for Repeat transparency
        if (const auto rep = dynamic_cast<const RepeatDecorator*>(cursor)) {
            const Node* inner = rep->Debug_Inner();
            if (!inner) break;
            cursor = inner;
            continue;
        }
        const auto* comp = dynamic_cast<const Composite*>(cursor);
        if (!comp) break;
        const auto idx = CurrentChildIndex(cursor);
        if (!idx) break;
        const Node* child = comp->Debug_Child(*idx);
        if (!child) break;
        cursor = child;
    }
    return last ? last->DebugName() : "?";
}


} // namespace AI::BT
