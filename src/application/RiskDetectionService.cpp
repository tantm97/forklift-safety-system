#include "forklift/application/RiskDetectionService.h"

#include <sstream>

namespace forklift::application {

namespace {

std::string make_alert_id(const std::string& camera_id,
                          int forklift_track,
                          int person_track,
                          domain::Alert::TimePoint ts) {
    using namespace std::chrono;
    const auto ns = duration_cast<nanoseconds>(ts.time_since_epoch()).count();
    std::ostringstream oss;
    oss << camera_id << ":F" << forklift_track << ":P" << person_track << ":" << ns;
    return oss.str();
}

}  // namespace

std::vector<domain::Alert> RiskDetectionService::evaluate(
        const std::string& camera_id,
        const std::vector<domain::Detection>& detections,
        domain::Alert::TimePoint frame_ts) const {

    std::vector<domain::Detection> persons;
    std::vector<domain::Detection> forklifts;
    persons.reserve(detections.size());
    forklifts.reserve(detections.size());

    for (const auto& d : detections) {
        switch (d.cls) {
            case domain::ObjectClass::kPerson:   persons.push_back(d);   break;
            case domain::ObjectClass::kForklift: forklifts.push_back(d); break;
            default: break;
        }
    }

    auto zones = zone_service_.zones_for(forklifts);

    std::vector<domain::Alert> alerts;
    for (const auto& zone : zones) {
        for (const auto& person : persons) {
            if (!zone.overlaps(person.box)) continue;

            domain::Alert a;
            a.alert_id          = make_alert_id(camera_id, zone.forklift_track_id,
                                                person.track_id, frame_ts);
            a.camera_id         = camera_id;
            a.type              = domain::AlertType::kPersonNearForklift;
            a.severity          = domain::AlertSeverity::kWarning;
            a.timestamp         = frame_ts;
            a.person_track_id   = person.track_id;
            a.forklift_track_id = zone.forklift_track_id;
            a.person_box        = person.box;
            a.forklift_box      = zone.forklift_box;
            a.safety_zone_box   = zone.zone_box;
            a.distance_px       = 0.0F;  // overlapping ⇒ 0
            alerts.push_back(std::move(a));
        }
    }
    return alerts;
}

}  // namespace forklift::application
