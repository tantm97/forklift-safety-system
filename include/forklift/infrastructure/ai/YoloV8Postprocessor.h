// YOLOv8 post-processing: tensor decoding, score filtering, NMS.
// Lives in infrastructure because it is model-format specific.

#ifndef FORKLIFT_INFRASTRUCTURE_AI_YOLO_V8_POSTPROCESSOR_H_
#define FORKLIFT_INFRASTRUCTURE_AI_YOLO_V8_POSTPROCESSOR_H_

#include <unordered_map>
#include <vector>

#include "forklift/domain/Detection.h"
#include "forklift/domain/ObjectClass.h"

namespace forklift::infrastructure::ai {

struct PostprocessParams {
    int   model_input_width {640};
    int   model_input_height{640};
    int   source_width      {0};
    int   source_height     {0};
    float conf_threshold    {0.35F};
    float nms_threshold     {0.45F};
    int   max_detections    {300};
    // Map model class indices to domain ObjectClass.
    // Default supplied by the constructor of the postprocessor user.
    std::unordered_map<int, domain::ObjectClass> class_map;
};

// Decode a raw YOLOv8 output tensor (layout: [1, 4 + num_classes, num_anchors]).
// `raw` points to floats; `num_classes` is the count after the 4 box channels.
std::vector<domain::Detection>
postprocess_yolov8(const float* raw,
                   std::size_t  num_anchors,
                   std::size_t  num_classes,
                   const PostprocessParams& params);

}  // namespace forklift::infrastructure::ai

#endif  // FORKLIFT_INFRASTRUCTURE_AI_YOLO_V8_POSTPROCESSOR_H_
