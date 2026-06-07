// Port: InferenceEngine — abstract object detector.
// Implementations: OnnxInferenceEngine, TensorRTInferenceEngine.
// The application layer depends ONLY on this header; concrete engines are
// wired in main.cpp via factory selection driven by config.

#ifndef FORKLIFT_APPLICATION_INFERENCE_ENGINE_H_
#define FORKLIFT_APPLICATION_INFERENCE_ENGINE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "forklift/domain/Detection.h"
#include "forklift/domain/Frame.h"
#include "forklift/domain/ObjectClass.h"
#include "forklift/shared/Result.h"

namespace forklift::application {

// Compute device an engine should target. Orthogonal to the backend: ONNX
// Runtime can run on either; TensorRT always implies kCuda.
enum class InferenceDevice { kCpu, kCuda };

[[nodiscard]] constexpr const char* to_string(InferenceDevice d) noexcept {
    return d == InferenceDevice::kCuda ? "cuda" : "cpu";
}

struct InferenceConfig {
    std::string     model_path;
    int             input_width   {640};
    int             input_height  {640};
    float           conf_threshold{0.35F};
    float           nms_threshold {0.45F};
    int             max_detections{300};
    InferenceDevice device        {InferenceDevice::kCpu};

    // Maps raw model class indices → domain ObjectClass. When empty, the engine
    // falls back to a built-in default. Driven by config so a re-trained model
    // (or a COCO stand-in mapping vehicles → forklift) can be wired without code
    // changes. Unmapped indices are dropped as kUnknown.
    std::unordered_map<int, domain::ObjectClass> class_map;
};

class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;

    // Load model + warm up. MUST be called once before infer().
    virtual shared::Result<void> initialize(const InferenceConfig& cfg) = 0;

    // Run inference on a single frame. Thread-safety contract:
    //   - One engine instance per worker thread, OR
    //   - The implementation declares itself thread-safe (see derived class docs).
    [[nodiscard]] virtual shared::Result<std::vector<domain::Detection>>
        infer(const domain::Frame& frame) = 0;

    [[nodiscard]] virtual const char* backend_name() const noexcept = 0;
};

}  // namespace forklift::application

#endif  // FORKLIFT_APPLICATION_INFERENCE_ENGINE_H_
