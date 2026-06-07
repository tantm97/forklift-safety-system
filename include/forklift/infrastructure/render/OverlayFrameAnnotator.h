// OverlayFrameAnnotator — OpenCV implementation of the FrameAnnotator port.
// Draws forklift boxes, expanded safety zones, person boxes, and confidence
// labels onto a copy of the frame for the live viewer.

#ifndef FORKLIFT_INFRASTRUCTURE_RENDER_OVERLAY_FRAME_ANNOTATOR_H_
#define FORKLIFT_INFRASTRUCTURE_RENDER_OVERLAY_FRAME_ANNOTATOR_H_

#include <vector>

#include "forklift/application/FrameAnnotator.h"

namespace forklift::infrastructure::render {

class OverlayFrameAnnotator final : public application::FrameAnnotator {
public:
    [[nodiscard]] domain::Frame annotate(
        const domain::Frame&                   frame,
        const std::vector<domain::Detection>&  detections,
        const std::vector<domain::SafetyZone>& zones) const override;
};

}  // namespace forklift::infrastructure::render

#endif  // FORKLIFT_INFRASTRUCTURE_RENDER_OVERLAY_FRAME_ANNOTATOR_H_
