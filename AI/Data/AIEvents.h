#pragma once

/// @brief Data structures for AI event representation

#include <Play.h>

namespace AI
{
struct ArrivedEvent   { Play::Vector2D goal{}; };
struct BlockedEvent   { Play::Vector2D at{}; };
struct SpottedEvent   { std::uint32_t id{0}; };
struct LostSightEvent { std::uint32_t id{0}; };

struct SoundHeardEvent {
  Play::Vector2D center{};
  float radius{0.f};
  enum class Kind {
    None,
    Tank,
    Bullet
   } kind{Kind::None};
};
} // namespace AI
