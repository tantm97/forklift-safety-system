// RiskDetectionService — pure domain service. Given the per-frame detections,
// splits them into persons[] and forklifts[], builds safety zones via
// SafetyZoneService, and emits an Alert for every (person, zone) pair where
// the person box overlaps the zone.
//
// Stateless across frames. Cross-frame de-duplication / cooldown is the
// responsibility of FrameProcessingPipeline (see notes there).

#ifndef FORKLIFT_APPLICATION_RISK_DETECTION_SERVICE_H_
#define FORKLIFT_APPLICATION_RISK_DETECTION_SERVICE_H_

#include <string>
#include <vector>

#include "forklift/application/SafetyZoneService.h"
#include "forklift/domain/Alert.h"
#include "forklift/domain/Detection.h"

namespace forklift::application {

class RiskDetectionService {
public:
    explicit RiskDetectionService(const SafetyZoneService& zone_service)
        : zone_service_(zone_service) {}

    [[nodiscard]] std::vector<domain::Alert>
        evaluate(const std::string& camera_id,
                 const std::vector<domain::Detection>& detections,
                 domain::Alert::TimePoint frame_ts) const;

private:
    const SafetyZoneService& zone_service_;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_RISK_DETECTION_SERVICE_H_
