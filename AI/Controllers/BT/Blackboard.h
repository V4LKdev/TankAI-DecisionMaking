#pragma once

/// @brief Blackboard data structure for Behavior Tree AI controllers.

#include "AI/Data/AIContext.h"
#include <optional>

namespace AI::BT {

struct Blackboard {
    // Mirrors
    SelfState self{};

    // Knowledge from AIDecisionController base
    std::optional<std::uint32_t> targetId{};
    std::optional<Play::Vector2D> lastKnown{};

    // Timers/flags (seconds)
    float damagedTimer{0.0f};      // time since last damage
    float fireTimer{0.0f};         // fire cadence cooldown
    float searchTimer{0.0f};       // time spent in search behavior (optional/debug)
    float soundTimer{0.0f};        // time since last sound processed

    // High HP behavior mode (random selection each cycle) - currently unused externally but kept for future
    enum class HighHPMode : unsigned char { None, Patrol, Look };
    HighHPMode highHpMode{HighHPMode::None};

    // Complex look-around phases
    int   lookPhase{0}; // 0 idle,1 left,2 pause,3 right
    float lookPhaseTimer{0.0f};
    float lookPhaseDuration{0.0f};

    void ClearTransient() {
        damagedTimer = 0.0f;
        fireTimer = 0.0f;
        searchTimer = 0.0f;
        soundTimer = 0.0f;
        highHpMode = HighHPMode::None;
        lookPhase = 0; lookPhaseTimer = 0.0f; lookPhaseDuration = 0.0f;
    }

    void ClearAll() {
        ClearTransient();
        targetId.reset();
        lastKnown.reset();
    }
};

} // namespace AI::BT
