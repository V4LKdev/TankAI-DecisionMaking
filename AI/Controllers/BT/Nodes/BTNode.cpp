#include "BTNode.h"
#include "AI/Gateway/AIServiceGateway.h"

namespace AI::BT {

/**
 * @brief Selector node in a behavior tree. Executes children nodes until one succeeds or all fail.
 */
Status Selector::Tick(float dt, Blackboard& bb, AI::AIServiceGateway& gw) {
    if (children_.empty()) { Debug_SetLastStatus_(Status::Failure); return Status::Failure; }

    for (std::size_t i = 0; i < children_.size(); ++i) {
        const auto & child = children_[i];
        const Status s = child->Tick(dt, bb, gw);
        if (s == Status::Failure) continue;

        if (s == Status::Running) {
            if (i != current_) {
                if (current_ < children_.size()) children_[current_]->OnExit(bb, gw);
                child->OnEnter(bb, gw);
                current_ = i;
            }
            Debug_SetLastStatus_(Status::Running);
            return Status::Running;
        }
        // Success: ensure child exit and clear current
        if (current_ < children_.size()) children_[current_]->OnExit(bb, gw);
        current_ = children_.size();
        Debug_SetLastStatus_(Status::Success);
        return Status::Success;
    }

    if (current_ < children_.size()) children_[current_]->OnExit(bb, gw);
    current_ = children_.size();
    Debug_SetLastStatus_(Status::Failure);
    return Status::Failure;
}

/**
 * @brief Sequence node in a behavior tree. Executes children nodes in order until one fails or all succeed.
 */
Status Sequence::Tick(float dt, Blackboard& bb, AIServiceGateway& gw) {
    if (children_.empty()) { Debug_SetLastStatus_(Status::Success); return Status::Success; }

    // If we've progressed beyond last child, sequence succeeded
    if (current_ >= children_.size()) {
        Debug_SetLastStatus_(Status::Success);
        return Status::Success;
    }

    const auto & child = children_[current_];
    const Status s = child->Tick(dt, bb, gw);
    if (s == Status::Running) {
        Debug_SetLastStatus_(Status::Running);
        return Status::Running;
    }

    if (s == Status::Failure) {
        // Exit current child and fail; restart sequence next tick from first child
        child->OnExit(bb, gw);
        Debug_SetLastStatus_(Status::Failure);
        current_ = 0; // ready to re-evaluate from start on next tick
        return Status::Failure;
    }

    // Child succeeded: exit it, advance to next, enter next if any, and continue Running
    child->OnExit(bb, gw);
    ++current_;
    if (current_ < children_.size()) {
        children_[current_]->OnEnter(bb, gw);
        Debug_SetLastStatus_(Status::Running);
        return Status::Running;
    }

    // All children succeeded this tick
    Debug_SetLastStatus_(Status::Success);
    // Leave current_ at children_.size() to mark completion
    return Status::Success;
}

} // namespace AI::BT
