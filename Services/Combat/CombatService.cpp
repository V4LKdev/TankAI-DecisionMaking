#include "Services/Combat/CombatService.h"
#include "CoreTank/Tank.h"

namespace Combat {

void CombatService::BindTank(Tank* tank) { tank_ = tank; }

void CombatService::SetProfile(const CombatConfig& p) { profile_ = p; }

const CombatConfig& CombatService::GetProfile() const { return profile_; }

void CombatService::SetOnFired(FiredCB cb) { onFired_ = std::move(cb); }

void CombatService::BeginCharge() { wantCharging_ = true; }

void CombatService::Release()     { wantCharging_ = false; }

void CombatService::Tick(const float dt, const AI::SelfState& /*self*/) {
  if (!tank_) return;

  tank_->Shoot(wantCharging_, dt);

  // Mirror charge estimate for AI/debug
  if (wantCharging_) chargeAccum_ += dt;

  // Charged last frame, released this frame -> assume fired
  if (prevCharging_ && !wantCharging_) {
    if (onFired_) onFired_();
    if (soundEmit_) {
      const auto sid = static_cast<std::uint32_t>(tank_->GetID());
      const Play::Vector2D pos = tank_->GetPosition();
      soundEmit_(sid, pos, 250.0f);
    }
    chargeAccum_ = 0.f;
  }

  prevCharging_ = wantCharging_;
}

} // namespace Combat
