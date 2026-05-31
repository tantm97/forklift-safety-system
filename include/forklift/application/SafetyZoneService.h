// SafetyZoneService — pure domain service. Given a forklift detection it computes
// the expanded SafetyZone according to a configured padding policy.
//
// Two padding modes are supported:
//   - kAbsolutePixels:  pad_x_px / pad_y_px are added on each side
//   - kRelativeToBox:   each side is padded by pad_ratio * (corresponding dimension)
//
// Choice belongs in config because it depends on camera FOV and forklift size.

#ifndef FORKLIFT_APPLICATION_SAFETY_ZONE_SERVICE_H_
#define FORKLIFT_APPLICATION_SAFETY_ZONE_SERVICE_H_

#include <vector>

#include "forklift/domain/Detection.h"
#include "forklift/domain/SafetyZone.h"

namespace forklift::application {

struct SafetyZoneConfig {
    enum class Mode { kAbsolutePixels, kRelativeToBox };

    Mode  mode{Mode::kRelativeToBox};
    float pad_x_px{120.0F};
    float pad_y_px{120.0F};
    float pad_ratio{0.5F};   // 0.5 ⇒ box grows by 50% of its width/height per side
};

class SafetyZoneService {
public:
    explicit SafetyZoneService(SafetyZoneConfig cfg) : cfg_(cfg) {}

    [[nodiscard]] domain::SafetyZone
        zone_for_forklift(const domain::Detection& forklift) const noexcept;

    [[nodiscard]] std::vector<domain::SafetyZone>
        zones_for(const std::vector<domain::Detection>& forklifts) const;

private:
    SafetyZoneConfig cfg_;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_SAFETY_ZONE_SERVICE_H_
