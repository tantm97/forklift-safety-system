# AI extensibility — changing the model, the device, or the whole method

This is the practical guide for the AGENTS.md goal: *the AI module is separate
from the stream, you can easily change the model, change the inference method,
and inject a new AI into the flow later.*

## The seam

The pipeline depends on exactly one contract:

```cpp
// include/forklift/application/InferenceEngine.h
class InferenceEngine {
 public:
  virtual shared::Result<void>            initialize(const InferenceConfig&) = 0;
  virtual shared::Result<std::vector<domain::Detection>> infer(const domain::Frame&) = 0;
  virtual std::string                     backend_name() const = 0;
};
```

`FrameProcessingPipeline` never names a concrete engine, ONNX Runtime, TensorRT,
or OpenCV. It only sees `InferenceEngine`. Everything below is swappable without
touching application or domain code.

```
VideoSource ──► Frame ──► [ InferenceEngine.infer() ] ──► Detection[] ──► RiskDetectionService
   (stream)                      (AI)                                         (safety logic)
```

The stream (`VideoSource`: RTSP or video file) and the AI (`InferenceEngine`)
are independent ports. Neither knows about the other.

## 1. Change the model

No code change. Point the config at a new ONNX file:

```yaml
inference:
  model_path: models/yolov8s_forklift.onnx
```

⚠️ The class-index map (`0 = person`, `1 = forklift`) is set in
`OnnxInferenceEngine::initialize`. A model trained with a different class order
will silently produce wrong alerts. If you retrain with a different order, file
a change to lift the map into config (tracked in ADR-0002 / ADR-0005).

## 2. Change the device (CPU ↔ CUDA)

No code change. The device is orthogonal to the backend:

```yaml
inference:
  backend: onnxruntime
  device: cuda          # cpu (default) | cuda  ("gpu" is an alias)
```

CUDA requires a CUDA-enabled ONNX Runtime and a build with `-DFSS_ORT_WITH_CUDA=ON`
(this is what the Jetson installer does with `--cuda`). On a CPU-only build the
engine logs a warning and falls back to CPU. See
[ADR-0005](../adr/0005-backend-device-selection.md).

## 3. Change the inference method / inject a new AI

The single, documented extension point is the factory:

```
include/forklift/infrastructure/ai/InferenceEngineFactory.h
src/infrastructure/ai/InferenceEngineFactory.cpp
```

To add a new AI (a different runtime, a quantised model family, or even a remote
inference microservice client):

1. **Implement the port.** Create
   `src/infrastructure/ai/MyInferenceEngine.cpp` (+ header) implementing
   `application::InferenceEngine`. Keep it self-contained behind a build flag if
   it pulls a new third-party dependency (mirror `FSS_WITH_ONNXRUNTIME`).
2. **Register it in the factory.** Add a `Backend` enum value, extend
   `parse_backend`, and construct your engine in `make_inference_engine`. Add it
   to `available_backends`.
3. **Select it in config.** `inference.backend: my-runtime`.

That is the whole change. `FrameProcessingPipeline`, `RiskDetectionService`,
`main.cpp`, and every test stay untouched — they only know the port.

### Why a factory and not a `switch` in `main.cpp`

A factory gives one obvious file to edit, a place to centralise
`(backend, device)` validation and fallback, and a list (`available_backends`)
the CLI/logs can print. See [ADR-0005](../adr/0005-backend-device-selection.md).

## Testing a new engine

Unit tests use a **fake** `InferenceEngine` (never a real model). Your adapter
gets its own integration check; the pipeline tests keep using the fake. See
[`docs/development/conventions/testing.md`](../development/conventions/testing.md)
and [`docs/development/conventions/inference.md`](../development/conventions/inference.md).
