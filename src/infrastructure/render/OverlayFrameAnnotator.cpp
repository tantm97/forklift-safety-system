#include "forklift/infrastructure/render/OverlayFrameAnnotator.h"

#include <string>

#include <opencv2/imgproc.hpp>

namespace forklift::infrastructure::render {

namespace {

// BGR colours (OpenCV order).
const cv::Scalar kPersonColor{0, 200, 0};      // green
const cv::Scalar kForkliftColor{0, 165, 255};  // orange
const cv::Scalar kZoneColor{0, 0, 255};        // red
const cv::Scalar kTextColor{255, 255, 255};    // white

cv::Rect to_rect(const domain::BoundingBox& b, const cv::Size& bounds) {
    int x = static_cast<int>(b.x);
    int y = static_cast<int>(b.y);
    int w = static_cast<int>(b.width);
    int h = static_cast<int>(b.height);
    // Clamp to image bounds so OpenCV never throws on out-of-range rects.
    x = std::max(0, std::min(x, bounds.width - 1));
    y = std::max(0, std::min(y, bounds.height - 1));
    w = std::max(0, std::min(w, bounds.width - x));
    h = std::max(0, std::min(h, bounds.height - y));
    return cv::Rect{x, y, w, h};
}

void draw_label(cv::Mat& img, const cv::Rect& box, const std::string& text,
                const cv::Scalar& color) {
    constexpr int    kFont      = cv::FONT_HERSHEY_SIMPLEX;
    constexpr double kScale     = 0.5;
    constexpr int    kThickness = 1;
    int baseline = 0;
    const cv::Size sz = cv::getTextSize(text, kFont, kScale, kThickness, &baseline);
    const int top = std::max(box.y, sz.height + 4);
    const cv::Point origin{box.x, top};
    cv::rectangle(img, origin + cv::Point{0, baseline},
                  origin + cv::Point{sz.width, -sz.height}, color, cv::FILLED);
    cv::putText(img, text, origin, kFont, kScale, kTextColor, kThickness, cv::LINE_AA);
}

}  // namespace

domain::Frame OverlayFrameAnnotator::annotate(
        const domain::Frame&                   frame,
        const std::vector<domain::Detection>&  detections,
        const std::vector<domain::SafetyZone>& zones) const {
    domain::Frame out = frame;
    out.image = frame.image.clone();  // never mutate the safety-path frame
    if (out.image.empty()) return out;

    const cv::Size bounds = out.image.size();

    // Safety zones first so detections render on top.
    for (const auto& zone : zones) {
        cv::rectangle(out.image, to_rect(zone.zone_box, bounds), kZoneColor, 2);
        cv::rectangle(out.image, to_rect(zone.forklift_box, bounds), kForkliftColor, 2);
    }

    for (const auto& det : detections) {
        const bool is_person = det.cls == domain::ObjectClass::kPerson;
        const cv::Scalar color = is_person ? kPersonColor : kForkliftColor;
        const cv::Rect rect = to_rect(det.box, bounds);
        cv::rectangle(out.image, rect, color, 2);

        std::string label{domain::to_string(det.cls)};
        label += " ";
        label += std::to_string(static_cast<int>(det.confidence * 100.0F));
        label += "%";
        draw_label(out.image, rect, label, color);
    }

    return out;
}

}  // namespace forklift::infrastructure::render
