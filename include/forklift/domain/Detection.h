// A single object detection produced by the inference stage.

#ifndef FORKLIFT_DOMAIN_DETECTION_H_
#define FORKLIFT_DOMAIN_DETECTION_H_

#include "forklift/domain/BoundingBox.h"
#include "forklift/domain/ObjectClass.h"

namespace forklift::domain {

struct Detection {
    BoundingBox  box{};
    ObjectClass  cls{ObjectClass::kUnknown};
    float        confidence{0.0F};  // [0, 1]
    int          track_id{-1};      // -1 when no tracker is attached
};

}  // namespace forklift::domain

#endif  // FORKLIFT_DOMAIN_DETECTION_H_
