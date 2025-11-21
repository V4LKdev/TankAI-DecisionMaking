#pragma once

/// @brief Follows a path by generating motion commands to reach each waypoint in sequence.

#include "AI/Data/AIContext.h"
#include "Services/Motion/Types.h"

namespace Motion
{
class PathFollower {
public:
  void SetProfile(const MotionConfig& p) { profile_ = p; }
  void SetPath(const AI::Path& p);
  [[nodiscard]] bool HasPath() const;
  void Cancel();
  FollowCommand Tick(const AI::SelfState& self);

private:
  MotionConfig profile_{};
  AI::Path path_{};

  // Internal state
  std::size_t currentSegmentIndex_{0}; // index i means segment from path[i] to path[i+1]

  // One-time pre-alignment: rotate on the spot towards initial heading before moving
  bool initialAlign_{false};
};
}
