#include "forklift/infrastructure/ai/YoloV8Postprocessor.h"

#include <algorithm>
#include <vector>

#include <opencv2/dnn.hpp>

namespace forklift::infrastructure::ai {

std::vector<domain::Detection>
postprocess_yolov8(const float* raw,
                   std::size_t  num_anchors,
                   std::size_t  num_classes,
                   const PostprocessParams& params) {
    // Layout: [1, 4 + num_classes, num_anchors] (YOLOv8 export default)
    // Channel order: cx, cy, w, h, class_0, class_1, ..., class_{C-1}
    const float scale_x = (params.source_width  > 0)
                              ? static_cast<float>(params.source_width)
                                    / static_cast<float>(params.model_input_width)
                              : 1.0F;
    const float scale_y = (params.source_height > 0)
                              ? static_cast<float>(params.source_height)
                                    / static_cast<float>(params.model_input_height)
                              : 1.0F;

    std::vector<cv::Rect>   boxes;
    std::vector<float>      scores;
    std::vector<int>        class_ids;
    boxes.reserve(num_anchors);
    scores.reserve(num_anchors);
    class_ids.reserve(num_anchors);

    const std::size_t stride = num_anchors;
    for (std::size_t i = 0; i < num_anchors; ++i) {
        int   best_cls   = -1;
        float best_score = 0.0F;
        for (std::size_t c = 0; c < num_classes; ++c) {
            const float s = raw[(4 + c) * stride + i];
            if (s > best_score) { best_score = s; best_cls = static_cast<int>(c); }
        }
        if (best_score < params.conf_threshold || best_cls < 0) continue;

        const float cx = raw[0 * stride + i];
        const float cy = raw[1 * stride + i];
        const float w  = raw[2 * stride + i];
        const float h  = raw[3 * stride + i];

        cv::Rect r(
            static_cast<int>((cx - w * 0.5F) * scale_x),
            static_cast<int>((cy - h * 0.5F) * scale_y),
            static_cast<int>(w * scale_x),
            static_cast<int>(h * scale_y));
        boxes.push_back(r);
        scores.push_back(best_score);
        class_ids.push_back(best_cls);
    }

    std::vector<int> keep;
    cv::dnn::NMSBoxes(boxes, scores, params.conf_threshold, params.nms_threshold, keep);

    std::vector<domain::Detection> out;
    const int max_keep = std::min(static_cast<int>(keep.size()), params.max_detections);
    out.reserve(static_cast<std::size_t>(max_keep));
    for (int k = 0; k < max_keep; ++k) {
        const int idx = keep[static_cast<std::size_t>(k)];
        domain::Detection d;
        d.box.x       = static_cast<float>(boxes[idx].x);
        d.box.y       = static_cast<float>(boxes[idx].y);
        d.box.width   = static_cast<float>(boxes[idx].width);
        d.box.height  = static_cast<float>(boxes[idx].height);
        d.confidence  = scores[idx];

        auto it = params.class_map.find(class_ids[idx]);
        d.cls = (it == params.class_map.end()) ? domain::ObjectClass::kUnknown : it->second;
        out.push_back(d);
    }
    return out;
}

}  // namespace forklift::infrastructure::ai
