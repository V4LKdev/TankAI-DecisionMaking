#pragma once

/// @brief Computes per-frame perception (vision/hearing) and maintains short-term memory for AI queries.

#include <vector>
#include <optional>
#include <cstdint>
#include <memory>
#include <Play.h>
#include "Services/Sensing/Types.h"

namespace AI { struct SelfState; struct Contact; }

namespace Sensing {

namespace Vision { class FOV; class LOS; }
namespace Memory { class Store; }

class SensingService {
public:
  SensingService();
  ~SensingService();

  void SetConfig(const SenseConfig& cfg);
  [[nodiscard]] const SenseConfig& GetConfig() const;

  // Main tick (call once per frame)
  void Update(float dt, const AI::SelfState& self);

  // Vision queries
  [[nodiscard]] PerceptionSnapshot VisibleNow(const std::vector<AI::Contact>& candidates) const;

  // LOS utility
  static bool HasLOS(const Play::Vector2D& a, const Play::Vector2D& b);

  // Audio integration
  void RegisterSound(std::uint32_t sourceId, const Play::Vector2D& pos, float radius) const;

  // Memory access
  void RememberSeen(std::uint32_t id, const Play::Vector2D& pos) const;
  void ForgetTarget(std::uint32_t id) const;
  
  [[nodiscard]] std::optional<AI::Contact> LastKnown(std::uint32_t id) const;

  // Debug helper: richer last-known info (source, age, uncertainty)
  [[nodiscard]] std::optional<LastKnownInfo> Debug_LastKnownInfo(std::uint32_t id) const;

private:
  SenseConfig    cfg_{};
  float          timeAccum_ = 0.f;
  AI::SelfState  self_{};       // copied each Update, avoidnig dangling pointer risk
  // Modules (owned)
  std::unique_ptr<Vision::FOV>   fov_{};
  std::unique_ptr<Vision::LOS>   los_{};
  std::unique_ptr<Memory::Store> store_{};
};

} // namespace Sensing
