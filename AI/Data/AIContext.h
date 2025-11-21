#pragma once

/// @brief Data structures for AI context representation

#include <Play.h>
#include <vector>
#include <cstdint>

namespace AI
{
using Path = std::vector<Play::Vector2D>;

struct SelfState {
  Play::Vector2D pos{};
  float rot{0.f};
  float radius{16.f};
  int   hp{100};
  // Unique identifier for this agent
  std::uint32_t id{0};
};

struct NavConstraints {
  Play::Vector2D center{};
  float radius{0.f};
};

struct ProjectionResult {
  Play::Vector2D projected{};
  bool valid{false};
};

struct Contact {
  std::uint32_t id{0};
  Play::Vector2D pos{};
  Play::Vector2D vel{};
  float lastSeenSec{0.f};
  float confidence{1.f}; // 0..1
  enum class Type {
    None,
    Tank,
    Projectile,
    Obstacle
  };
  Type type{Type::None};
};

struct PerceptionSnapshot {
  std::vector<Contact> visible;
  float timestampSec{0.f};
};
} // namespace AI
