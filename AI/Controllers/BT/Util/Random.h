#pragma once

/// @brief Utility functions for random number generation in behavior trees.
// TODO: should be made global and used for other systems as well.

#include <random>

namespace AI::BT::Rand {

inline std::mt19937& Engine() {
    static std::mt19937 eng{ std::random_device{}() };
    return eng;
}

inline float Float(const float minV, const float maxV) {
    std::uniform_real_distribution<float> dist(minV, maxV);
    return dist(Engine());
}

inline bool Chance(float p) {
    if (p <= 0.0f) return false;
    if (p >= 1.0f) return true;
    std::bernoulli_distribution dist(p);
    return dist(Engine());
}

} // namespace AI::BT::Rand

