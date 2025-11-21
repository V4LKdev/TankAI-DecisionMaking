// ReSharper disable CppDFAConstantConditions
#include "AIServiceGateway.h"
#include "Combat/CombatService.h"
#include "Play.h"
#include "Services/Pathfinding/Pathfinding.h"
#include "Services/Motion/Motion.h"
#include "Services/Sensing/Sensing.h"
#include "Services/Sensing/Audio/Bus.h"
#include "CoreTank/Tank.h"
#include "Helper/Geometry.h"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <cassert>

namespace AI
{
    // ---- Ctor ----
    AIServiceGateway::AIServiceGateway(Pathfinding::PathfinderService& pf,
                                       Motion::MotionService* motion,
                                       Sensing::SensingService* sensing,
                                       Combat::CombatService* combat,
                                       Sensing::Audio::Bus* audioBus,
                                       const bool emitSounds)
    : pf_(pf), motion_(motion), sensing_(sensing), combat_(combat), audioBus_(audioBus), emitSounds_(emitSounds) {
        BindMotionCallbacks_(); // I guess this is the syntax we are using now xD
    }

    void AIServiceGateway::BindMotionCallbacks_() {
        if (!motion_) return;

        motion_->SetCallbacks(
            // Arrived
            [this](const Play::Vector2D& g)
            {
                if (subs_.onArrived) subs_.onArrived(ArrivedEvent{ g });
                debugCounts_.arrived++;
            },
            // Blocked
            [this](const Play::Vector2D& at)
            {
                if (subs_.onBlocked) subs_.onBlocked(BlockedEvent{ at });
                debugCounts_.blocked++;
            }
        );
        BindSoundEmitter_();
    }

    void AIServiceGateway::BindSoundEmitter_() const {
      if (!audioBus_ || !emitSounds_) return; // CLion is determined that audioBus_ is always null here smh
      // ReSharper disable once CppDFAUnreachableCode
      if (motion_) {
            motion_->SetSoundEmitter([this](const std::uint32_t sourceId, const Play::Vector2D& pos, const float loudness){
                 Sensing::Audio::BusEvent e{}; e.sourceId = sourceId; e.pos = pos;
                 e.radius = loudness;
                 e.kind = SoundHeardEvent::Kind::Tank;
                 // ReSharper disable once CppDFANullDereference
                 audioBus_->Push(e);
            });
        }
        if (combat_) {
            combat_->SetSoundEmitter([this](const std::uint32_t sourceId, const Play::Vector2D& pos, const float loudness){
                 Sensing::Audio::BusEvent e{}; e.sourceId = sourceId; e.pos = pos;
                 e.radius = loudness;
                 e.kind = SoundHeardEvent::Kind::Bullet;
                // ReSharper disable once CppDFANullDereference
                 audioBus_->Push(e);
            });
        }
    }

    void AIServiceGateway::TickSensing(const float dt)
    {
        if (!sensing_) return;
        assert((audioBus_ != nullptr) || (emitSounds_ == false));
        sensing_->Update(dt, self_);

        // Visible diff (spotted / lost sight)
        const auto currentVisible = Sense_VisibleEnemies();
        std::unordered_set<std::uint32_t> currVisibleIds;
        currVisibleIds.reserve(currentVisible.size());
        for (const auto& c : currentVisible) currVisibleIds.insert(c.id);

        // Spotted: in curr but not prev
        for (const auto& id : currVisibleIds) {
            if (!prevVisibleIds_.contains(id)) {
                if (subs_.onSpotted) subs_.onSpotted(SpottedEvent{ id });
                debugCounts_.spotted++;
            }
        }

        // Lost sight: in prev but not curr
        for (const auto& id : prevVisibleIds_) {
            if (!currVisibleIds.contains(id)) {
                if (subs_.onLostSight) subs_.onLostSight(LostSightEvent{ id });
                debugCounts_.lost++;
            }
        }

        // Replace previous set atomically
        prevVisibleIds_.swap(currVisibleIds);

        // Always scan audio for counting, emit onSound only if subscribed
        if (audioBus_)
        {
            const auto nearby = audioBus_->QueryInRadius(self_.pos);
            for (const auto &ev : nearby) {
                if (ev.sourceId == self_.id) continue;
                const std::uint64_t key = (static_cast<std::uint64_t>(ev.sourceId) << 32) | static_cast<std::uint64_t>(ev.kind);

                // Process each bus event at most once per listener by sequence id
                std::uint32_t &lastSeq = lastProcessedSeq_[key];
                if (ev.seq <= lastSeq) continue; // already processed this event instance
                lastSeq = ev.seq;

                // Time-based debounce (secondary guard)
                float &last = soundDebounceTimers_[key];
                const float minInterval = (ev.kind == SoundHeardEvent::Kind::Bullet) ? 1.0f : 0.5f;
                const auto now = static_cast<float>(::GetTime());
                if ((now - last) < minInterval) continue;
                last = now;

                if (!prevVisibleIds_.contains(ev.sourceId)) {
                    if (sensing_) sensing_->RegisterSound(ev.sourceId, ev.pos, ev.radius);
                }
                if (subs_.onSound) {
                    SoundHeardEvent e{}; e.center = ev.pos; e.radius = ev.radius; e.kind = ev.kind;
                    subs_.onSound(e);
                }
                debugCounts_.sounds++;
            }
        }
    }

    // ---- Low-level intents ----
    void AIServiceGateway::Drive(const int intent) { if (motion_) motion_->Move(intent); }
    void AIServiceGateway::Turn(const int intent)  { if (motion_) motion_->Rotate(intent); }

    // ---- Nav ----
    ProjectionResult AIServiceGateway::Nav_Project(const Play::Vector2D& p) const
    {
        ProjectionResult r;

        const auto& config = pf_.GetConfig();
        const auto [minx, miny, maxx, maxy]   = Pathfinding::GetOuterPlayableRect(config);

        // Clamp point to the playable outer rectangle
        const Play::Vector2D clamped{
            std::clamp(p.x, minx, maxx),
            std::clamp(p.y, miny, maxy)
        };

        r.projected = clamped;
        r.valid     = Pathfinding::PointInOuterPlayable(p, config);
        return r;
    }

    Path AIServiceGateway::Nav_FindPath(const Play::Vector2D& start,const Play::Vector2D& goal) const
    {
        if (auto result = pf_.PlanPath(start, goal))
        {
            return result->polyline;
        }
        return {};
    }

    Play::Vector2D AIServiceGateway::Nav_GetRandomReachable(const AI::NavConstraints& c) const
    {
        return pf_.GetRandomReachablePoint();
    }

    // ---- Sensing: helpers ----
    static std::vector<Contact> BuildEnemyCandidates_(const SelfState& self)
    {
        std::vector<Contact> out;
        out.reserve(Tank::GetAllTanks().size());
        for (const auto * t : Tank::GetAllTanks()) {
            if (!t || !t->IsAlive()) continue;
            // Exclude self by id if known
            if (static_cast<std::uint32_t>(t->GetID()) == self.id) continue;
            Contact c{};
            c.id  = static_cast<std::uint32_t>(t->GetID());
            c.pos = t->GetPosition();
            c.type = Contact::Type::Tank;
            out.push_back(c);
        }
        return out;
    }

    // ---- Sensing ----
    std::vector<Contact> AIServiceGateway::Sense_VisibleEnemies() const {
        if (!sensing_) return {};
        const auto candidates = BuildEnemyCandidates_(self_);
        const auto [visible] = sensing_->VisibleNow(candidates);
        return visible;
    }

    bool AIServiceGateway::Sense_HasLOS(const Play::Vector2D& a,const Play::Vector2D& b) const {
        if (!sensing_) return false;
        return Sensing::SensingService::HasLOS(a, b);
    }

    std::optional<Contact> AIServiceGateway::Sense_LastKnown(const std::uint32_t id) const {
        if (!sensing_) return std::nullopt;
        return sensing_->LastKnown(id);
    }

    // ---- Intents ----
    void AIServiceGateway::MoveTo(const Play::Vector2D& goal) {
        if (!motion_)
        {
            if (subs_.onBlocked) subs_.onBlocked(BlockedEvent{ self_.pos });
            return;
        }

        // Case 1: Start ~ Goal (arrive shortcut)
        const float tol = std::max(10.0f, 0.5f * self_.radius);
        if (Geom::dist(self_.pos, goal) <= tol)
        {
            if (subs_.onArrived) subs_.onArrived(ArrivedEvent{ goal });
            return;
        }

        // Case 2: Plan a path
        const Path path = Nav_FindPath(self_.pos, goal);
        if (path.empty())
        {
            // Case 2b: Planner failed (no route)
            if (subs_.onBlocked) subs_.onBlocked(BlockedEvent{ self_.pos });
            return;
        }

        // Align motion lookahead to pathfinding params (approx 2x turn radius, safely clamped)
        {
            const auto& params = pf_.GetConfig();
            const float baseL = std::clamp(params.turnRadius * 2.0f, 24.0f, 120.0f);
            Motion::MotionConfig prof; // start from defaults
            prof.lookahead_base = baseL;
            motion_->SetProfile(prof);
        }

        // Case 3: Valid path -> follow
        motion_->FollowPath(path);
    }

    void AIServiceGateway::MoveToRandom(const NavConstraints& c) {
        const Play::Vector2D target = Nav_GetRandomReachable(c);
        MoveTo(target);
    }

    void AIServiceGateway::CancelMove() {
        if (motion_) motion_->CancelFollow();
    }

    void AIServiceGateway::Stop() {
        if (motion_)
        {
            motion_->CancelFollow();
            motion_->Stop();
        }
    }

    // ---- Intents (Motion) ----
    void AIServiceGateway::AimAt(const Play::Vector2D& target) {
        if (motion_) motion_->AimAt(target);
    }

    void AIServiceGateway::CancelAim() {
        if (motion_) motion_->CancelAim();
    }

    // ---- Queries (Motion) ----
    bool AIServiceGateway::IsAiming() const {
        return motion_ ? motion_->IsAiming() : false;
    }

    bool AIServiceGateway::IsOnTarget() const {
        return motion_ ? motion_->IsOnTarget() : false;
    }

    Motion::FollowCommand::Status AIServiceGateway::GetMotionStatus() const {
        return motion_ ? motion_->GetStatus() : Motion::FollowCommand::Status::Idle;
    }

    void AIServiceGateway::BeginFire() { if (combat_) combat_->BeginCharge();  }
    void AIServiceGateway::ReleaseFire() { if (combat_) combat_->Release();    }

    bool AIServiceGateway::Combat_IsCharging() const { return combat_ ? combat_->IsCharging() : false; }
    float AIServiceGateway::Combat_ChargeAccum() const { return combat_ ? combat_->ChargeAccum() : 0.0f; }

    // ---- Context & subscriptions ----
    void AIServiceGateway::SetSelfState(const SelfState& s) { self_ = s; }
    // ---- Signals ----
    void AIServiceGateway::SetSubscriptions(const Subscriptions& subs) { subs_ = subs; }
    void AIServiceGateway::NotifyDamageTaken(int amount) {
        if (subs_.onDamage) subs_.onDamage(amount);
    }
}
