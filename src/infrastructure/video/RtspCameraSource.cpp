#include "forklift/infrastructure/video/RtspCameraSource.h"

#include "forklift/infrastructure/logging/Logger.h"

namespace forklift::infrastructure::video {

RtspCameraSource::RtspCameraSource(RtspConfig cfg) : cfg_(std::move(cfg)) {}
RtspCameraSource::~RtspCameraSource() { close(); }

shared::Result<void> RtspCameraSource::open() {
    cap_.open(cfg_.rtsp_url, cv::CAP_FFMPEG);
    if (!cap_.isOpened()) {
        return shared::Error{1, "failed to open RTSP: " + cfg_.rtsp_url};
    }
    // Keep the OpenCV buffer small to minimise latency. Real-time pipelines
    // always want the most-recent frame, not a historic one.
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
    LOG_INFO("rtsp opened camera=" << cfg_.camera_id);
    return {};
}

void RtspCameraSource::close() {
    if (cap_.isOpened()) {
        cap_.release();
        LOG_INFO("rtsp closed camera=" << cfg_.camera_id);
    }
}

bool RtspCameraSource::is_open() const { return cap_.isOpened(); }

std::optional<domain::Frame> RtspCameraSource::read_frame() {
    if (!cap_.isOpened()) return std::nullopt;
    cv::Mat mat;
    if (!cap_.read(mat) || mat.empty()) return std::nullopt;
    domain::Frame f;
    f.camera_id   = cfg_.camera_id;
    f.sequence    = ++sequence_;
    f.captured_at = domain::Frame::Clock::now();
    f.image       = std::move(mat);
    return f;
}

}  // namespace forklift::infrastructure::video
