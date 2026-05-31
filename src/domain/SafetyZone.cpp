#include "forklift/domain/SafetyZone.h"

namespace forklift::domain {

bool SafetyZone::contains_point(float px, float py) const noexcept {
    return px >= zone_box.x && px <= zone_box.right() &&
           py >= zone_box.y && py <= zone_box.bottom();
}

bool SafetyZone::overlaps(const BoundingBox& other) const noexcept {
    return zone_box.intersects(other);
}

}  // namespace forklift::domain
