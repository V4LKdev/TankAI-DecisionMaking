#pragma once

/// @brief Manages charge/shot lifecycle for a tank; integrates with sensing via optional firing sound emitter.

#include "Play.h"
#include "Services/Combat/Types.h"

#include <cstdint>
#include <functional>

class Tank;

namespace AI { struct SelfState; }

namespace Combat {

class CombatService {
public:
  void BindTank(Tank* tank);
  void SetProfile(const CombatConfig& p);
  [[nodiscard]] const CombatConfig& GetProfile() const;

  void SetOnFired(FiredCB cb);

  // Audio: allow gateway to hook firing sounds
  using SoundEmitCB = std::function<void(std::uint32_t /*sourceId*/, const Play::Vector2D& pos, float loudness)>;
  void SetSoundEmitter(const SoundEmitCB& cb) { soundEmit_ = cb; }

  // Intents (edge-driven)
  void BeginCharge();  // press/hold
  void Release();      // release -> fire on this frame

  void Tick(float dt, const AI::SelfState& self);

  // Introspection
  [[nodiscard]] bool  IsCharging()   const { return wantCharging_; }
  [[nodiscard]] float ChargeAccum()  const { return chargeAccum_;  }

private:
  Tank*          tank_         = nullptr;
  CombatConfig  profile_{};

  bool  wantCharging_ = false;   // desired input this frame
  bool  prevCharging_ = false;
  float chargeAccum_  = 0.f;     // mirror for AI/debug (Tank owns truth)
  FiredCB onFired_{};
  SoundEmitCB soundEmit_{};
};

} // namespace Combat
