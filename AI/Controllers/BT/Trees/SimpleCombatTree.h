#pragma once

/// @brief Behavior tree: Guarded HighHP selector (Engage, Search, Random Patrol/LookAround); LowHP -> GuardThreat(Flee) else loop MicroPatrol->LookAround.

#include "AI/Controllers/BT/Nodes/BTNode.h"
#include "AI/Controllers/BT/BTConfig.h"
#include "AI/Controllers/BT/Nodes/Conditions.h"
#include "AI/Controllers/BT/Nodes/Actions.h"
#include "AI/Controllers/BT/Nodes/RandomSelector.h"
#include "AI/Controllers/BT/Nodes/Decorators.h"
#include <memory>

namespace AI::BT {

inline void AssignIdsRec(Node* n, std::uint32_t& nextId) {
    if (!n) return;
    n->Debug_AssignId(nextId++);
    if (const auto comp = dynamic_cast<Composite*>(n)) {
        for (std::size_t i = 0; i < comp->Debug_ChildCount(); ++i) {
            AssignIdsRec(const_cast<Node*>(comp->Debug_Child(i)), nextId);
        }
    }
}

inline std::unique_ptr<Node> BuildSimpleCombatTree(const Config& cfg = Config{}) {
    auto root = std::make_unique<Selector>();

    // High HP branch
    auto guardHigh = root->AddChild<GuardHighHP>(cfg);
    {
        auto inner = guardHigh->AddChild<Selector>();
        // ├── Engage
        {
            auto seq = inner->AddChild<Sequence>();
            seq->AddChild<ConditionTargetVisible>(); // └── Condition: Target Visible
            seq->AddChild<EngageAction>(cfg);        //     └── Action: Engage
        }
        // ├── Search
        {
            auto seq = inner->AddChild<Sequence>();
            seq->AddChild<ConditionHasLastKnown>(); // └── Condition: Has Last Known Position
            seq->AddChild<SearchAction>();         //     └── Action: Search
        }
        // └── Random Patrol/LookAround
        auto rnd = inner->AddChild<RandomSelector>();
        rnd->AddChild<PatrolAction>();            // ├── Action: Patrol
        rnd->AddChild<LookAroundAction>(cfg);     // └── Action: Look Around
    }

    // Low HP branch
    auto guardLow = root->AddChild<GuardLowHP>(cfg);
    {
        auto sel = guardLow->AddChild<Selector>();
        // ├── Guarded flee
        {
            auto guardThreat = sel->AddChild<GuardThreat>();
            guardThreat->AddChild<FleeAction>();  // └── Action: Flee
        }
        // └── Loop cautious behavior
        {
            auto seqNode = std::make_unique<Sequence>();
            seqNode->AddChild<MicroPatrolAction>(cfg); // ├── Action: Micro Patrol
            seqNode->AddChild<LookAroundAction>(cfg);  // └── Action: Look Around
            sel->AddChild<RepeatDecorator>(std::move(seqNode));
        }
    }

    std::uint32_t nextId = 1;
    AssignIdsRec(root.get(), nextId);
    return root;
}

} // namespace AI::BT
