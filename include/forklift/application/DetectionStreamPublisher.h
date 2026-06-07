// DetectionStreamPublisher — application port for streaming per-frame detection
// data (bounding boxes + safety zones) to the UI tier.
//
// The implementation (MjpegStreamServer) serialises detections to JSON and
// delivers them via SSE so the browser canvas can draw overlays without the
// C++ layer burning boxes into the MJPEG frames. All calls are best-effort and
// never block the safety path.

#ifndef FORKLIFT_APPLICATION_DETECTION_STREAM_PUBLISHER_H_
#define FORKLIFT_APPLICATION_DETECTION_STREAM_PUBLISHER_H_

#include <string>
#include <vector>

#include "forklift/domain/Detection.h"
#include "forklift/domain/SafetyZone.h"

namespace forklift::application {

class DetectionStreamPublisher {
public:
    // Publish one frame's worth of detections. Implementation must be
    // thread-safe (called from inference worker threads). Best-effort: returns
    // void and must never throw.
    virtual void publish_detections(
        const std::string&                      camera_id,
        const std::vector<domain::Detection>&   detections,
        const std::vector<domain::SafetyZone>&  zones,
        int                                     frame_w,
        int                                     frame_h) = 0;

    virtual ~DetectionStreamPublisher() = default;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_DETECTION_STREAM_PUBLISHER_H_
