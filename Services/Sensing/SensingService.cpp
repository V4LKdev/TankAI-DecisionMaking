#include "Services/Sensing/SensingService.h"
#include "Services/Sensing/Vision/FOV.h"
#include "Services/Sensing/Vision/LOS.h"
#include "Services/Sensing/Memory/Store.h"
#include "Data/AIContext.h"

namespace Sensing {

SensingService::SensingService() {
    fov_   = std::make_unique<Vision::FOV>();
    los_   = std::make_unique<Vision::LOS>();
    store_ = std::make_unique<Memory::Store>();
}

SensingService::~SensingService() = default;

void SensingService::SetConfig(const SenseConfig& cfg) {
    cfg_ = cfg;
}

const SenseConfig& SensingService::GetConfig() const { return cfg_; }

void SensingService::Update(const float dt, const AI::SelfState& self) {
    timeAccum_ += dt;
    self_ = self;
    // Decay memory
    store_->Decay(dt, cfg_.memoryTTL);
}

PerceptionSnapshot SensingService::VisibleNow(const std::vector<AI::Contact>& candidates) const {
    PerceptionSnapshot snap;

    if (fov_ && los_) {
        Vision::FOV::Compute(self_, candidates, *los_, cfg_, snap.visible);
        // Update memory for visible contacts (source = Vision, uncertainty = 0)
        for (const auto& c : snap.visible) {
            RememberSeen(c.id, c.pos);
        }
    }
    return snap;
}

bool SensingService::HasLOS(const Play::Vector2D& a, const Play::Vector2D& b) {
    return Vision::LOS::HasLineOfSight(a, b);
}

void SensingService::RegisterSound(const std::uint32_t sourceId, const Play::Vector2D& pos, const float radius) const {
	store_->RememberHeard(sourceId, pos, radius);
}

void SensingService::RememberSeen(const std::uint32_t id, const Play::Vector2D& pos) const
{
    store_->RememberSeen(id, pos);
}

void SensingService::ForgetTarget(const std::uint32_t id) const
{
    store_->Forget(id);
}



std::optional<AI::Contact> SensingService::LastKnown(const std::uint32_t id) const {
    if (const auto info = store_->LastKnown(id)) {
        AI::Contact c{}; c.id = info->id; c.pos = info->pos; return c;
    }
    return std::nullopt;
}

std::optional<LastKnownInfo> SensingService::Debug_LastKnownInfo(const std::uint32_t id) const {
    return store_->LastKnown(id);
}

} // namespace Sensing
