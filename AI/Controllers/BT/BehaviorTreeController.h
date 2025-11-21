#pragma once

/// @brief Behavior Tree AI decision controller.

#include "AI/Controllers/AIDecisionController.h"
#include "AI/Controllers/BT/Blackboard.h"
#include "AI/Controllers/BT/BTConfig.h"
#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Controllers/BT/Trees/SimpleCombatTree.h"
#include <memory>
#include <functional>
#include <string>

namespace AI::BT {

class BehaviorTreeController final : public AI::AIDecisionController {
public:
    using AIDecisionController::AIDecisionController;

    // Inject a custom tree builder (defaults to BuildSimpleCombatTree)
    void SetTreeBuilder(std::function<std::unique_ptr<Node>()> builder) { builder_ = std::move(builder); }

    void Update(float dt) override;
    void SetActive(bool on) override;

    // Debug utilities
    [[nodiscard]] const char* Debug_GetRootName() const { return root_ ? root_->DebugName() : "NoTree"; }
    [[nodiscard]] std::vector<std::uint32_t> Debug_CurrentPathIds() const;
    [[nodiscard]] std::string Debug_CurrentLeafName() const;

protected:
    // Event hooks
    void onSpotted_(const AI::SpottedEvent& e) override;
    void onLostSight_(const AI::LostSightEvent& e) override;
    void onSound_(const AI::SoundHeardEvent& e) override;
    void onDamage_(int amount) override;

private:
    void buildTree_();
    bool wasAlive_{true};

    Blackboard bb_{};
    Config cfg_{};

    std::function<std::unique_ptr<Node>()> builder_{[this]{ return BuildSimpleCombatTree(cfg_); }};
    std::unique_ptr<Node> root_{}; // root node
};

} // namespace AI::BT
