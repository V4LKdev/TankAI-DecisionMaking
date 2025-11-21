#pragma once

#include "AI/Gateway/AIServiceGateway.h"
#include "Helper/Geometry.h"
#include <optional>
#include <random>
#include <cmath>

namespace AI::Behaviors {

inline void PatrolToRandomPoint(AI::AIServiceGateway& gw) {
    gw.CancelAim();
    gw.MoveTo(gw.Nav_GetRandomReachable({}));
}


// TODO: This is much too complex for what it does...
// Helper to find a point away from a threat, projected onto the walkable map
inline Play::Vector2D FindFleeLocation(const Play::Vector2D& selfPos, 
                                       const std::optional<Play::Vector2D>& threatPos,
                                       AIServiceGateway& gateway) {
    
Play::Vector2D fleeVector;
thread_local std::mt19937 rng{ std::random_device{}() };

if (threatPos.has_value()) {
    // Base is away from threat
    fleeVector = { selfPos.x - threatPos->x, selfPos.y - threatPos->y };
} else {
    // No threat: pick a fully random direction
    std::uniform_real_distribution<float> angDist(0.0f, 2.0f * Play::PLAY_PI);
    float a = angDist(rng);
    fleeVector = { std::cos(a), std::sin(a) };
}

// Guard zero-length vectors
float len = std::hypot(fleeVector.x, fleeVector.y);
if (len < 1e-3f) {
    std::uniform_real_distribution<float> angDist(0.0f, 2.0f * Play::PLAY_PI);
    float a = angDist(rng);
    fleeVector = { std::cos(a), std::sin(a) };
    len = 1.0f;
}

// Normalize
Play::Vector2D dir = Geom::norm(fleeVector);

// If fleeing from a known threat, deviate up to 60 degrees from the exact opposite direction
if (threatPos.has_value()) {
    constexpr float kMaxDeviation = Play::PLAY_PI / 3.0f; // 60 degrees
    std::uniform_real_distribution<float> devDist(0.0f, kMaxDeviation);
    float deviation = devDist(rng);
    std::uniform_int_distribution<int> signDist(0, 1);
    if (signDist(rng)) deviation = -deviation;
    const float c = std::cos(deviation);
    const float s = std::sin(deviation);
    dir = { dir.x * c - dir.y * s, dir.x * s + dir.y * c };
}

// Extend to flee distance and project to walkable area
constexpr float kFleeDistance = 1000.0f; // Pixels to attempt to flee
const Play::Vector2D idealFleePoint{
    selfPos.x + dir.x * kFleeDistance,
    selfPos.y + dir.y * kFleeDistance
};
return gateway.Nav_Project(idealFleePoint).projected;
}

} // namespace AI::Behaviors