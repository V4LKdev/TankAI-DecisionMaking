#pragma once

/// @brief Base class for AI debug layers.

namespace AI {
class AISubsystem;

class DebugLayer {
public:
  virtual ~DebugLayer() = default;
  DebugLayer() = default;

  virtual void render(const AISubsystem& sys) const = 0;
  virtual void handleInput(const AISubsystem& sys) = 0;
  virtual void toggleVisibility() { visible_ = !visible_; }
  [[nodiscard]] virtual bool isVisible() const { return visible_; }
protected:
  bool visible_ = false;
};

} // namespace AI