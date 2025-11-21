#pragma once

/// @brief Basic 2D geometry utilities.

#include <Play.h>
#include <cmath>
#include <algorithm>

namespace Geom {

// Length (magnitude) of a 2D vector
inline float len(const Play::Vector2D& v) {
  return std::sqrt(v.x*v.x + v.y*v.y);
}

// Safe normalization; returns zero vector if the input is too small
inline Play::Vector2D norm(const Play::Vector2D& v) {
  const float L = len(v);
  return (L > 1e-6f) ? Play::Vector2D{ v.x / L, v.y / L } : Play::Vector2D{ 0, 0 };
}

// Dot and cross products
inline float dot(const Play::Vector2D& a, const Play::Vector2D& b) {
  return a.x*b.x + a.y*b.y;
}
inline float cross(const Play::Vector2D& a, const Play::Vector2D& b) {
  return a.x*b.y - a.y*b.x;
}

// Clamp 'x' to the [lo, hi] range
inline float clampf(const float x, const float lo, const float hi) {
  return std::max(lo, std::min(hi, x));
}

// Angle (radians) of vector measured from +X (CCW)
inline float angOf(const Play::Vector2D& v) {
  return std::atan2(v.y, v.x);
}

// Squared and linear distance between two points
inline float dist2(const Play::Vector2D& a, const Play::Vector2D& b) {
  const float dx = b.x - a.x;
  const float dy = b.y - a.y;
  return dx*dx + dy*dy;
}
inline float dist(const Play::Vector2D& a, const Play::Vector2D& b) {
  return std::sqrt(dist2(a, b));
}

// Wrap angle to [-pi, pi]
inline float wrapAngle(float a) {
  while (a > Play::PLAY_PI)  a -= 2.0f * Play::PLAY_PI;
  while (a < -Play::PLAY_PI) a += 2.0f * Play::PLAY_PI;
  return a;
}

} // namespace Geom

