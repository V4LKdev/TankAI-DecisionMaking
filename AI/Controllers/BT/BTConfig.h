#pragma once

/// @brief Configuration parameters for Behavior Tree AI controllers.

namespace AI::BT {

struct Config {
    // Timings
    float fleeMinDurationSec{1.5f};
    float fleeCooldownSec{4.0f};
    float searchTimeoutSec{3.0f};
    float lookAroundBurstSec{1.0f};
    float patrolRepathSec{2.5f};
    float fireCadenceSec{0.35f};
    float lowHpMicroPatrolSec{2.0f};

    // Combat / engage radii
    float engageAttackRadius{240.0f};
    float engageChaseRadius{300.0f};

    // Health threshold for low HP branch
    int lowHpThreshold{1};

    // Look-around timing ranges (seconds)
    float look_idle_min{0.4f};
    float look_idle_max{0.8f};
    float look_rotate_min{0.8f};
    float look_rotate_max{1.3f};
    float look_pause_min{0.4f};
    float look_pause_max{0.7f};

    // Chance (0..1) to swap rotation order
    float look_swap_dir_chance{0.5f};
};
}
