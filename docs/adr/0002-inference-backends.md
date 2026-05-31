# ADR-0002: Support ONNX Runtime and TensorRT side-by-side

**Date:**     2026-05-23
**Status:**   Accepted
**Category:** Architecture
**Change:**   bootstrap

## Context

Target hardware varies: CPU-only edge boxes (ORT-CPU), Intel iGPU (ORT with
OpenVINO EP), and NVIDIA Jetson / discrete GPUs (TensorRT). Forcing a single
backend would exclude entire deployment classes.

## Decision

- Introduce `application::InferenceEngine` as the only contract the pipeline
  knows about (`initialize`, `infer`, `backend_name`).
- Provide two implementations:
  - `infrastructure::ai::OnnxInferenceEngine` (default, gated by
    `FSS_WITH_ONNXRUNTIME`, ON by default)
  - `infrastructure::ai::TensorRTInferenceEngine` (gated by
    `FSS_WITH_TENSORRT`, OFF by default — opt-in for NVIDIA targets)
- Selection happens in the composition root from `backend:` in
  `conf/system.yaml`.
- Both backends share `YoloV8Postprocessor` (model-format-specific, not
  runtime-specific) so adding a third backend is bounded work.

## Consequences

- ✅ The same binary can target multiple deployment classes by build flag.
- ✅ Pipeline code is unchanged when adding a new backend.
- ⚠️ Two CMake options to keep in sync with CI matrices.
- ⚠️ Class index → `ObjectClass` map is set in each engine's `initialize` —
  duplication risk. Mitigation: extract to a `ClassMapLoader` in a future
  ADR when the second backend lands.
