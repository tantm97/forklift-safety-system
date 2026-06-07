#include "forklift/infrastructure/ai/InferenceEngineFactory.h"

#ifdef FSS_WITH_ONNXRUNTIME
#include "forklift/infrastructure/ai/OnnxInferenceEngine.h"
#endif

namespace forklift::infrastructure::ai {

Backend parse_backend(const std::string& name) noexcept {
    if (name == "tensorrt" || name == "trt") return Backend::kTensorRT;
    return Backend::kOnnxRuntime;
}

const char* to_string(Backend backend) noexcept {
    switch (backend) {
        case Backend::kTensorRT:    return "tensorrt";
        case Backend::kOnnxRuntime: return "onnxruntime";
    }
    return "onnxruntime";
}

std::vector<std::string> available_backends() {
    std::vector<std::string> out;
#ifdef FSS_WITH_ONNXRUNTIME
    out.emplace_back("onnxruntime");
#endif
    // TensorRT is reserved (see docs/adr/0002-inference-backends.md): the adapter
    // is a stub, so it is not advertised as available yet.
    return out;
}

shared::Result<std::shared_ptr<application::InferenceEngine>>
make_inference_engine(Backend backend) {
    // ── Extension point ──────────────────────────────────────────────────────
    // To inject a new AI method: add an enum value in the header, a CMake guard
    // (FSS_WITH_<BACKEND>), and a case below that returns your adapter. Nothing
    // else in the pipeline needs to change.
    switch (backend) {
        case Backend::kOnnxRuntime:
#ifdef FSS_WITH_ONNXRUNTIME
            return std::static_pointer_cast<application::InferenceEngine>(
                std::make_shared<OnnxInferenceEngine>());
#else
            return shared::Error{
                20, "onnxruntime backend requested but binary built without "
                    "FSS_WITH_ONNXRUNTIME"};
#endif
        case Backend::kTensorRT:
            // Adapter is a stub today; wire it here once implemented.
            return shared::Error{
                21, "tensorrt backend is not yet implemented "
                    "(see docs/adr/0002-inference-backends.md)"};
    }
    return shared::Error{22, "unknown inference backend"};
}

}  // namespace forklift::infrastructure::ai
