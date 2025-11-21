#pragma once

/// @brief Base class for FSM states.

namespace AI {
class FSMController;

class FSMStateBase {
public:
  explicit FSMStateBase(FSMController& c) : ctrl_(c) {}
  virtual ~FSMStateBase() = default;

  virtual void OnEnter() {}
  virtual void Tick(float dt) = 0;
  virtual void OnExit() {}

protected:
  FSMController& ctrl_;
};

} // namespace AI

