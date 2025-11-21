#include "Services/Sensing/Vision/LOS.h"
#include "Helper/LineOfSight.h"

namespace Sensing::Vision {

bool LOS::HasLineOfSight(const Play::Vector2D& a, const Play::Vector2D& b) {
  return LOSHelper::HasLOS_RawStructures(a, b);
}

} // namespace Sensing::Vision
