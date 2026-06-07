// Port: FrameStreamPublisher — best-effort sink for annotated frames consumed by
// a live viewer (e.g. the MJPEG dashboard). This is intentionally separate from
// AlertPublisher: alerts are safety-critical events, whereas the frame stream is
// an operator convenience that may be dropped under back-pressure.
//
// Contract:
//   - publish() is non-blocking and never throws.
//   - Implementations may drop frames to protect the hot path.
//   - The target camera is identified by frame.camera_id.

#ifndef FORKLIFT_APPLICATION_FRAME_STREAM_PUBLISHER_H_
#define FORKLIFT_APPLICATION_FRAME_STREAM_PUBLISHER_H_

#include "forklift/domain/Frame.h"

namespace forklift::application {

class FrameStreamPublisher {
public:
    virtual ~FrameStreamPublisher() = default;

    virtual void publish(const domain::Frame& frame) = 0;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_FRAME_STREAM_PUBLISHER_H_
