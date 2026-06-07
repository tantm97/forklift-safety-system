// InferenceEngineFactory — the single place where concrete AI backends are
// constructed. This is the extension point for "inject a new AI method later":
// add a case here (and a CMake guard) and the rest of the system is unchanged,
// because everything upstream depends only on the application::InferenceEngine
// port.
//
// The factory returns an *uninitialised* engine; the caller is responsible for
// calling engine->initialize(cfg). It never selects the device — that lives in
// InferenceConfig and is honoured by each engine's initialize().

#ifndef FORKLIFT_INFRASTRUCTURE_AI_INFERENCE_ENGINE_FACTORY_H_
#define FORKLIFT_INFRASTRUCTURE_AI_INFERENCE_ENGINE_FACTORY_H_

#include <memory>
#include <string>
#include <vector>

#include "forklift/application/InferenceEngine.h"
#include "forklift/shared/Result.h"

namespace forklift::infrastructure::ai {

// Stable identifiers for the built-in backends. New backends extend this enum
// and the switch in make_inference_engine().
enum class Backend { kOnnxRuntime, kTensorRT };

[[nodiscard]] Backend parse_backend(const std::string& name) noexcept;
[[nodiscard]] const char* to_string(Backend backend) noexcept;

// Names of the backends that were actually compiled into this binary.
[[nodiscard]] std::vector<std::string> available_backends();

// Construct an uninitialised engine for the requested backend. Returns an error
// when the backend is unknown or was not compiled in (so callers get a clear
// message instead of a null pointer).
[[nodiscard]] shared::Result<std::shared_ptr<application::InferenceEngine>>
make_inference_engine(Backend backend);

}  // namespace forklift::infrastructure::ai

#endif  // FORKLIFT_INFRASTRUCTURE_AI_INFERENCE_ENGINE_FACTORY_H_
