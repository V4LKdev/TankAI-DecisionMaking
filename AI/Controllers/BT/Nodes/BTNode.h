#pragma once

/// @brief Behavior Tree node base and core composites (Selector, Sequence) with debug support.

#include <vector>
#include <memory>
#include <cstddef>
#include <optional>
#include <cstdint>

namespace AI { class AIServiceGateway; }
namespace AI::BT { struct Blackboard; }

namespace AI::BT {

enum class Status : unsigned char { Success, Failure, Running };

class Node {
public:
    virtual ~Node() = default;

    virtual Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) = 0;
    virtual void OnEnter(Blackboard&, AIServiceGateway&) {}
    virtual void OnExit(Blackboard&, AIServiceGateway&) {}

    // Debug: identity and structure
    [[nodiscard]] virtual const char* DebugName() const { return "Node"; }
    [[nodiscard]] Status Debug_LastStatus() const { return lastStatus_; }
    [[nodiscard]] std::uint32_t Debug_Id() const { return id_; }
    [[nodiscard]] const Node* Debug_Parent() const { return parent_; }
    void Debug_AssignId(std::uint32_t id) { id_ = id; }
    void Debug_SetParent_(Node* p) { parent_ = p; }

protected:
    void Debug_SetLastStatus_(Status s) { lastStatus_ = s; }
    void Debug_AssignId_(std::uint32_t id) { id_ = id; }

private:
    Node* parent_{nullptr};
    Status lastStatus_{Status::Failure};
    std::uint32_t id_{0};
};

class Composite : public Node {
protected:
    std::vector<std::unique_ptr<Node>> children_{};
public:
    template <typename T, typename... Args>
    T* AddChild(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = ptr.get();
        raw->Debug_SetParent_(this);
        children_.push_back(std::move(ptr));
        return raw;
    }

    [[nodiscard]] std::size_t Debug_ChildCount() const { return children_.size(); }
    [[nodiscard]] const Node* Debug_Child(const std::size_t i) const { return (i < children_.size()) ? children_[i].get() : nullptr; }
};

class Selector final : public Composite {
    std::size_t current_{0};
public:
    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override;
    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override { current_ = 0; if (!children_.empty()) children_[0]->OnEnter(bb, gw); }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override { if (current_ < children_.size()) children_[current_]->OnExit(bb, gw); }
    [[nodiscard]] const char* DebugName() const override { return "Selector"; }
    [[nodiscard]] std::optional<std::size_t> Debug_CurrentIndex() const { return (current_ < children_.size()) ? std::optional<std::size_t>(current_) : std::nullopt; }
};

class Sequence final : public Composite {
    std::size_t current_{0};
public:
    Status Tick(float dt, Blackboard& bb, AIServiceGateway& gw) override;
    void OnEnter(Blackboard& bb, AIServiceGateway& gw) override { current_ = 0; if (!children_.empty()) children_[0]->OnEnter(bb, gw); }
    void OnExit(Blackboard& bb, AIServiceGateway& gw) override { if (current_ < children_.size()) children_[current_]->OnExit(bb, gw); }
    [[nodiscard]] const char* DebugName() const override { return "Sequence"; }
    [[nodiscard]] std::optional<std::size_t> Debug_CurrentIndex() const { return (current_ < children_.size()) ? std::optional<std::size_t>(current_) : std::nullopt; }
};

} // namespace AI::BT
