#include "forklift/application/SafetyZoneService.h"

namespace forklift::application {

domain::SafetyZone SafetyZoneService::zone_for_forklift(
        const domain::Detection& forklift) const noexcept {
    domain::SafetyZone zone;
    zone.forklift_box      = forklift.box;
    zone.forklift_track_id = forklift.track_id;

    float pad_x = cfg_.pad_x_px;
    float pad_y = cfg_.pad_y_px;
    if (cfg_.mode == SafetyZoneConfig::Mode::kRelativeToBox) {
        pad_x = forklift.box.width  * cfg_.pad_ratio;
        pad_y = forklift.box.height * cfg_.pad_ratio;
    }
    zone.zone_box = forklift.box.expanded(pad_x, pad_y);
    return zone;
}

std::vector<domain::SafetyZone> SafetyZoneService::zones_for(
        const std::vector<domain::Detection>& forklifts) const {
    std::vector<domain::SafetyZone> out;
    out.reserve(forklifts.size());
    for (const auto& f : forklifts) {
        out.push_back(zone_for_forklift(f));
    }
    return out;
}

}  // namespace forklift::application
