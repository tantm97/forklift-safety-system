// SafetyZone — a forklift's bounding box expanded by a configurable margin.
// Decoupled from Detection so the same value can be applied to test fixtures
// or to mocked forklift positions.

#ifndef FORKLIFT_DOMAIN_SAFETY_ZONE_H_
#define FORKLIFT_DOMAIN_SAFETY_ZONE_H_

#include "forklift/domain/BoundingBox.h"

namespace forklift::domain {

struct SafetyZone {
    BoundingBox forklift_box{};   // original forklift detection box
    BoundingBox zone_box{};       // expanded box defining the danger area
    int         forklift_track_id{-1};

    [[nodiscard]] bool contains_point(float px, float py) const noexcept;
    [[nodiscard]] bool overlaps(const BoundingBox& other) const noexcept;
};

}  // namespace forklift::domain

#endif  // FORKLIFT_DOMAIN_SAFETY_ZONE_H_
