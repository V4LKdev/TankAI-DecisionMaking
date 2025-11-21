#pragma once

/// @brief Field of View (FOV) computations for vision sensing.

#include <vector>
#include "Services/Sensing/Types.h"

namespace AI { struct SelfState; struct Contact; }
namespace Sensing::Vision {

class LOS;

class FOV {
public:
  // Populate outVisible with candidates in cone & LOS
  static void Compute(const AI::SelfState& self,
               const std::vector<AI::Contact>& candidates,
               const LOS& los,
               const SenseConfig& cfg,
               std::vector<AI::Contact>& outVisible);
};

} // namespace Sensing::Vision
