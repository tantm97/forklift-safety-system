#include "forklift/infrastructure/video/VideoFileSource.h"

#include "forklift/infrastructure/logging/Logger.h"

namespace forklift::infrastructure::video {

VideoFileSource::VideoFileSource(VideoFileConfig cfg) : cfg_(std::move(cfg)) {}
VideoFileSource::~VideoFileSource() { close(); }

shared::Result<void> VideoFileSource::open() {
    cap_.open(cfg_.file_path);
    if (!cap_.isOpened()) {
        return shared::Error{1, "failed to open video file: " + cfg_.file_path};
    }
    LOG_INFO("video file opened camera=" << cfg_.camera_id << " path=" << cfg_.file_path);
    return {};
}

void VideoFileSource::close() {
    if (cap_.isOpened()) {
        cap_.release();
        LOG_INFO("video file closed camera=" << cfg_.camera_id);
    }
}

bool VideoFileSource::is_open() const { return cap_.isOpened(); }

std::optional<domain::Frame> VideoFileSource::read_frame() {
    if (!cap_.isOpened()) return std::nullopt;

    cv::Mat mat;
    if (!cap_.read(mat) || mat.empty()) {
        if (!cfg_.loop) return std::nullopt;

        // Rewind to the first frame.
        cap_.set(cv::CAP_PROP_POS_FRAMES, 0);
        if (!cap_.read(mat) || mat.empty()) return std::nullopt;

        LOG_DEBUG("video file looped camera=" << cfg_.camera_id);
    }

    domain::Frame f;
    f.camera_id   = cfg_.camera_id;
    f.sequence    = ++sequence_;
    f.captured_at = domain::Frame::Clock::now();
    f.image       = std::move(mat);
    return f;
}

}  // namespace forklift::infrastructure::video
