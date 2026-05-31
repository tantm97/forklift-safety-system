// Video-file adapter built on cv::VideoCapture.
//
// Suitable for offline testing: replays a local video file at the file's
// native frame-rate. When loop=true, seek is reset to the first frame after
// the last frame has been read.
//
// One instance per camera entry. Not thread-safe.

#ifndef FORKLIFT_INFRASTRUCTURE_VIDEO_VIDEO_FILE_SOURCE_H_
#define FORKLIFT_INFRASTRUCTURE_VIDEO_VIDEO_FILE_SOURCE_H_

#include <cstdint>
#include <string>

#include <opencv2/videoio.hpp>

#include "forklift/application/VideoSource.h"

namespace forklift::infrastructure::video {

struct VideoFileConfig {
    std::string camera_id;
    std::string file_path;
    bool        loop{true};
};

class VideoFileSource final : public application::VideoSource {
public:
    explicit VideoFileSource(VideoFileConfig cfg);
    ~VideoFileSource() override;

    shared::Result<void>                       open()       override;
    void                                       close()      override;
    [[nodiscard]] bool                         is_open() const override;
    [[nodiscard]] std::optional<domain::Frame> read_frame() override;
    [[nodiscard]] const std::string&           camera_id() const override { return cfg_.camera_id; }

private:
    VideoFileConfig  cfg_;
    cv::VideoCapture cap_;
    std::uint64_t    sequence_{0};
};

}  // namespace forklift::infrastructure::video

#endif  // FORKLIFT_INFRASTRUCTURE_VIDEO_VIDEO_FILE_SOURCE_H_
