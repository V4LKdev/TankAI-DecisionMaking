#pragma once

/// @brief Facade for controllers to issue intents and read queries across PF/Motion/Sensing/Combat; handles audio wiring and sense events.

#include "AI/Data/AIContext.h"
#include "AI/Data/AIEvents.h"
#include <functional>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <cstdint>
#include "Services/Motion/Types.h"

namespace Pathfinding { class PathfinderService; }
namespace Motion      { class MotionService;     }
namespace Sensing     { class SensingService;    }
namespace Combat      { class CombatService;     }
namespace Sensing::Audio { class Bus; }

namespace AI
{

class AIServiceGateway {
public:
    using ArrivedCB   = std::function<void(const ArrivedEvent&)>;
    using BlockedCB   = std::function<void(const BlockedEvent&)>;
    using SpottedCB   = std::function<void(const SpottedEvent&)>;
    using LostSightCB = std::function<void(const LostSightEvent&)>;
    using SoundCB     = std::function<void(const SoundHeardEvent&)>;
    using DamageCB    = std::function<void(int)>;

    struct Subscriptions {
        ArrivedCB   onArrived{};
        BlockedCB   onBlocked{};
        SpottedCB   onSpotted{};
        LostSightCB onLostSight{};
        SoundCB     onSound{};
        DamageCB    onDamage{};
    };

    struct DebugEventCounts {
        std::uint32_t spotted{0};
        std::uint32_t lost{0};
        std::uint32_t sounds{0};
        std::uint32_t arrived{0};
        std::uint32_t blocked{0};
    };

    explicit AIServiceGateway(Pathfinding::PathfinderService& pf,
                              Motion::MotionService* motion = nullptr,
                              Sensing::SensingService* sensing = nullptr,
                              Combat::CombatService* combat = nullptr,
                              Sensing::Audio::Bus* audioBus = nullptr,
                              bool emitSounds = false);

    // Reset transient per-agent state
    void Reset() { soundDebounceTimers_.clear(); prevVisibleIds_.clear(); debugCounts_ = {}; }

    // ---- Per frame ----
    void TickSensing(float dt);

    // ---- Queries (Nav) ----
    [[nodiscard]] ProjectionResult Nav_Project(const Play::Vector2D& p) const;
    [[nodiscard]] Path             Nav_FindPath(const Play::Vector2D& start, const Play::Vector2D& goal) const;
    [[nodiscard]] Play::Vector2D   Nav_GetRandomReachable(const NavConstraints& c) const;

    // ---- Queries (Sensing) ----
    [[nodiscard]] std::vector<Contact> Sense_VisibleEnemies() const;
    [[nodiscard]] bool                 Sense_HasLOS(const Play::Vector2D& a, const Play::Vector2D& b) const;
    [[nodiscard]] std::optional<Contact> Sense_LastKnown(std::uint32_t id) const;

    // ---- Intents ----
    void MoveTo(const Play::Vector2D& goal);
    void MoveToRandom(const NavConstraints& c);
    void CancelMove();
    void Stop();

    // Low-level intents
    void Drive(int intent);
    void Turn(int intent);

    // ---- Intents (Motion) ----
    void AimAt(const Play::Vector2D& target);
    void CancelAim();

    // ---- Queries (Motion) ----
    [[nodiscard]] bool IsAiming() const;
    [[nodiscard]] bool IsOnTarget() const;
    [[nodiscard]] Motion::FollowCommand::Status GetMotionStatus() const;

    void BeginFire();
    void ReleaseFire();

    // ---- Queries (Combat) ----
    [[nodiscard]] bool  Combat_IsCharging() const;
    [[nodiscard]] float Combat_ChargeAccum() const;

    // ---- Context ----
    void        SetSelfState(const SelfState& s); // controller sets each tick
    [[nodiscard]] const SelfState& Self() const { return self_; }

    // ---- Signals ----
    void SetSubscriptions(const Subscriptions& subs);
    void NotifyDamageTaken(int amount);

    // ---- Wiring ----
    void SetSoundEmissionEnabled(bool enabled) { emitSounds_ = enabled; BindSoundEmitter_(); }

    // ---- Debug accessors ----
    [[nodiscard]] DebugEventCounts Debug_GetEventCounts() const { return debugCounts_; }
    void Debug_ResetEventCounts() { debugCounts_ = {}; }

private:
    void BindMotionCallbacks_();
    void BindSoundEmitter_() const;

    Pathfinding::PathfinderService& pf_;
    Motion::MotionService*           motion_{nullptr};
    Sensing::SensingService*         sensing_{nullptr};
    Combat::CombatService*           combat_{nullptr};
    Sensing::Audio::Bus*             audioBus_{nullptr};
    bool emitSounds_{false};
    SelfState self_{};
    Subscriptions subs_{};

    std::unordered_set<std::uint32_t> prevVisibleIds_{};
    // Debounce timers for sounds: key = (sourceId<<32) | kind
    std::unordered_map<std::uint64_t, float> soundDebounceTimers_{};
    // Per-listener processed event seq to avoid duplicate handling of the same bus event
    std::unordered_map<std::uint64_t, std::uint32_t> lastProcessedSeq_{};

    // Debug tallies and last event times
    DebugEventCounts debugCounts_{};
 };


 } // namespace AI
