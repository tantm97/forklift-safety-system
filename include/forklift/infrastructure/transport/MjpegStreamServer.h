// MjpegStreamServer — serves the operator dashboard over HTTP. Implements both
// FrameStreamPublisher (raw MJPEG) and DetectionStreamPublisher (SSE per-frame
// detection data for browser canvas overlay).
//
// Endpoints:
//   GET /                            → dashboard (web_root/index.html)
//   GET /<asset>                     → static asset from web_root
//   GET /cameras                     → JSON array of camera ids with a live frame
//   GET /stream?camera=<id>          → multipart/x-mixed-replace MJPEG stream
//   GET /detections?camera=<id>      → text/event-stream SSE detection data
//
// Design: one io_context on a dedicated thread. publish() JPEG-encodes the
// latest frame per camera; publish_detections() serialises detections to JSON.
// Both are stored in SharedState; streaming sessions poll on a timer.
// The implementation lives behind a PIMPL so Boost.Beast never leaks to callers.

#ifndef FORKLIFT_INFRASTRUCTURE_TRANSPORT_MJPEG_STREAM_SERVER_H_
#define FORKLIFT_INFRASTRUCTURE_TRANSPORT_MJPEG_STREAM_SERVER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "forklift/application/DetectionStreamPublisher.h"
#include "forklift/application/FrameStreamPublisher.h"
#include "forklift/shared/Result.h"

namespace forklift::infrastructure::transport {

struct ViewerServerConfig {
    bool          enabled{false};
    std::string   host{"0.0.0.0"};
    std::uint16_t port{8088};
    std::string   web_root{"web"};   // directory served for the dashboard assets
    int           jpeg_quality{75};  // 1..100
    int           target_fps{15};    // stream cadence cap
};

class MjpegStreamServer final
    : public application::FrameStreamPublisher,
      public application::DetectionStreamPublisher {
public:
    explicit MjpegStreamServer(ViewerServerConfig cfg);
    ~MjpegStreamServer() override;

    shared::Result<void> start();
    void                 stop();

    // FrameStreamPublisher: JPEG-encode frame and push to MJPEG sessions.
    void publish(const domain::Frame& frame) override;

    // DetectionStreamPublisher: serialise detections to JSON and push to SSE sessions.
    void publish_detections(
        const std::string&                      camera_id,
        const std::vector<domain::Detection>&   detections,
        const std::vector<domain::SafetyZone>&  zones,
        int                                     frame_w,
        int                                     frame_h) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace forklift::infrastructure::transport

#endif  // FORKLIFT_INFRASTRUCTURE_TRANSPORT_MJPEG_STREAM_SERVER_H_
