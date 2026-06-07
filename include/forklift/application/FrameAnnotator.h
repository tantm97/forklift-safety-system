// Port: FrameAnnotator — draws detection + safety-zone overlays onto a frame
// for live viewing. Kept as a port so the drawing technology (OpenCV today)
// stays in infrastructure and the pipeline depends only on this interface.
//
// Implementations MUST NOT mutate the input frame; they return a new Frame so
// the original (used by the safety logic) is never altered.

#ifndef FORKLIFT_APPLICATION_FRAME_ANNOTATOR_H_
#define FORKLIFT_APPLICATION_FRAME_ANNOTATOR_H_

#include <vector>

#include "forklift/domain/Detection.h"
#include "forklift/domain/Frame.h"
#include "forklift/domain/SafetyZone.h"

namespace forklift::application {

class FrameAnnotator {
public:
    virtual ~FrameAnnotator() = default;

    [[nodiscard]] virtual domain::Frame annotate(
        const domain::Frame&                   frame,
        const std::vector<domain::Detection>&  detections,
        const std::vector<domain::SafetyZone>& zones) const = 0;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_FRAME_ANNOTATOR_H_
