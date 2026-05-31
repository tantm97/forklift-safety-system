// RTSP camera adapter built on cv::VideoCapture (FFmpeg backend).
//
// One instance per camera. Not thread-safe — the owning capture thread is the
// only thing allowed to call read_frame().

#ifndef FORKLIFT_INFRASTRUCTURE_VIDEO_RTSP_CAMERA_SOURCE_H_
#define FORKLIFT_INFRASTRUCTURE_VIDEO_RTSP_CAMERA_SOURCE_H_

#include <cstdint>
#include <string>

#include <opencv2/videoio.hpp>

#include "forklift/application/VideoSource.h"

namespace forklift::infrastructure::video {

struct RtspConfig {
    std::string camera_id;
    std::string rtsp_url;
    int         reconnect_backoff_ms{1000};
    int         max_reconnect_backoff_ms{15000};
    int         read_timeout_ms{5000};
};

class RtspCameraSource final : public application::VideoSource {
public:
    explicit RtspCameraSource(RtspConfig cfg);
    ~RtspCameraSource() override;

    shared::Result<void>            open()                 override;
    void                            close()                override;
    [[nodiscard]] bool              is_open() const        override;
    [[nodiscard]] std::optional<domain::Frame> read_frame() override;
    [[nodiscard]] const std::string& camera_id() const     override { return cfg_.camera_id; }

private:
    RtspConfig       cfg_;
    cv::VideoCapture cap_;
    std::uint64_t    sequence_{0};
};

}  // namespace forklift::infrastructure::video

#endif  // FORKLIFT_INFRASTRUCTURE_VIDEO_RTSP_CAMERA_SOURCE_H_
