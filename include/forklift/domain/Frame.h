// Frame — a decoded video frame plus capture metadata.
// Owns its pixel data via cv::Mat (reference-counted by OpenCV).

#ifndef FORKLIFT_DOMAIN_FRAME_H_
#define FORKLIFT_DOMAIN_FRAME_H_

#include <chrono>
#include <cstdint>
#include <string>

#include <opencv2/core/mat.hpp>

namespace forklift::domain {

struct Frame {
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    std::string  camera_id;
    std::uint64_t sequence{0};      // monotonic per-camera frame counter
    TimePoint    captured_at{};
    cv::Mat      image;             // BGR, 8-bit, may be empty on shutdown sentinel

    [[nodiscard]] bool empty() const noexcept { return image.empty(); }
};

}  // namespace forklift::domain

#endif  // FORKLIFT_DOMAIN_FRAME_H_
