#include "forklift/infrastructure/video/VideoFileSource.h"

#include <thread>

#include "forklift/infrastructure/logging/Logger.h"

namespace forklift::infrastructure::video {

VideoFileSource::VideoFileSource(VideoFileConfig cfg) : cfg_(std::move(cfg)) {}
VideoFileSource::~VideoFileSource() { close(); }

shared::Result<void> VideoFileSource::open() {
    cap_.open(cfg_.file_path);
    if (!cap_.isOpened()) {
        return shared::Error{1, "failed to open video file: " + cfg_.file_path};
    }

    double fps = cap_.get(cv::CAP_PROP_FPS);
    if (!(fps > 0.0) || fps > 240.0) fps = 25.0;  // guard NaN / bogus metadata
    frame_interval_ = std::chrono::milliseconds(static_cast<long>(1000.0 / fps));
    next_frame_at_  = std::chrono::steady_clock::now();

    LOG_INFO("video file opened camera=" << cfg_.camera_id << " path=" << cfg_.file_path
                                         << " fps=" << fps
                                         << " realtime=" << (cfg_.realtime_pacing ? 1 : 0));
    return {};
}

void VideoFileSource::close() {
    if (cap_.isOpened()) {
        cap_.release();
        LOG_INFO("video file closed camera=" << cfg_.camera_id);
    }
}

bool VideoFileSource::is_open() const { return cap_.isOpened(); }

void VideoFileSource::pace() {
    if (!cfg_.realtime_pacing || frame_interval_.count() <= 0) return;

    const auto now = std::chrono::steady_clock::now();
    if (now < next_frame_at_) {
        std::this_thread::sleep_for(next_frame_at_ - now);
        next_frame_at_ += frame_interval_;
    } else {
        // We fell behind (slow consumer / decode). Resync the schedule to now so
        // we don't burst-emit a backlog of frames trying to "catch up".
        next_frame_at_ = now + frame_interval_;
    }
}

std::optional<domain::Frame> VideoFileSource::read_frame() {
    if (!cap_.isOpened()) return std::nullopt;

    pace();

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
