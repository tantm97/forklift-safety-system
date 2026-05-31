// ONNX Runtime backend for YOLOv8.
// Compiled only when FSS_WITH_ONNXRUNTIME is defined (see CMakeLists.txt).

#ifndef FORKLIFT_INFRASTRUCTURE_AI_ONNX_INFERENCE_ENGINE_H_
#define FORKLIFT_INFRASTRUCTURE_AI_ONNX_INFERENCE_ENGINE_H_

#include <memory>
#include <vector>

#include "forklift/application/InferenceEngine.h"

#ifdef FSS_WITH_ONNXRUNTIME
namespace Ort {
struct Env;
struct Session;
struct MemoryInfo;
struct AllocatorWithDefaultOptions;
}  // namespace Ort
#endif

namespace forklift::infrastructure::ai {

// Thread-safety: NOT thread-safe — one instance per worker thread.
class OnnxInferenceEngine final : public application::InferenceEngine {
public:
    OnnxInferenceEngine();
    ~OnnxInferenceEngine() override;

    shared::Result<void> initialize(const application::InferenceConfig& cfg) override;

    [[nodiscard]] shared::Result<std::vector<domain::Detection>>
        infer(const domain::Frame& frame) override;

    [[nodiscard]] const char* backend_name() const noexcept override { return "onnxruntime"; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace forklift::infrastructure::ai

#endif  // FORKLIFT_INFRASTRUCTURE_AI_ONNX_INFERENCE_ENGINE_H_
