#pragma once

/// @brief Types and configurations for motion profiling and path following.

#include <Play.h>
#include <vector>
#include <functional>

namespace AI{
struct SelfState;
using Path = std::vector<Play::Vector2D>;
}

namespace Motion
{

// Configuration parameters for motion profiling
struct MotionConfig {
  float v_step{2.0f};
  float w_step{0.05f};
  float lookahead_base{56.0f};
  float ang_tol{0.04f};
  float arrive_tol_px{10.0f};
  int   stuck_frames{30};
  int   unstick_frames{20};
  int   max_unstick_attempts{3};
  float k_alpha{0.7f};
  float progress_eps{0.5f}; // minimum distance considered as progress
};

struct FollowCommand {
  float move{0.f};   // forward input
  float rotate{0.f}; // yaw input
  enum class Status {
    Idle,
    Following,
    Arrived,
    Blocked
  };
  Status status{Status::Idle};
  Play::Vector2D lookahead{};
};

using ArrivedCB = std::function<void(const Play::Vector2D& goal)>;
using BlockedCB = std::function<void(const Play::Vector2D& at)>;

} // namespace Motion
