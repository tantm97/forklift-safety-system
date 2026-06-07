# ADR-0005: Inference backend + device selection via a factory

**Date:**     2026-06-01
**Status:**   Accepted
**Category:** Architecture
**Change:**   add-inference-factory

## Context

[ADR-0002](0002-inference-backends.md) introduced the `InferenceEngine` port
with ONNX Runtime and TensorRT implementations selected by a `switch` in the
composition root. Two gaps emerged when targeting Jetson Nano alongside laptops:

1. **Device/EP selection** — the same ONNX Runtime backend should run on CPU
   (laptop default) or CUDA (Jetson), but there was no way to ask for an
   execution provider from config.
2. **Extensibility** — "inject a new AI later" was a stated goal, but a `switch`
   in `main.cpp` is an awkward, easy-to-miss extension point.

## Decision

- Add `application::InferenceDevice { kCpu, kCuda }` and a `device` field on
  `InferenceConfig`. Device is **orthogonal** to backend.
- `OnnxInferenceEngine` appends the CUDA execution provider when
  `device == kCuda` *and* the binary was built with `FSS_ORT_WITH_CUDA`;
  otherwise it falls back to CPU and logs a warning.
- Introduce `infrastructure::ai::InferenceEngineFactory` — the single,
  documented place that maps `(backend, device) → InferenceEngine`. It exposes
  `parse_backend`, `make_inference_engine`, and `available_backends`.
- The composition root calls the factory instead of a hand-written `switch`.
- YAML: `inference.backend` + `inference.device` (default `cpu`), backward
  compatible with the legacy top-level `backend:` key. `"gpu"` is an alias for
  `cuda`.

## Consequences

- ✅ One binary, one config, runs CPU on a laptop and CUDA on a Jetson by
  changing `inference.device`.
- ✅ Adding a new AI method (e.g. a different model family or a remote
  inference client) is a *bounded* change: implement the `InferenceEngine`
  port and register it in the factory — the pipeline and `main.cpp` are
  untouched. See
  [`docs/architecture/ai-extensibility.md`](../architecture/ai-extensibility.md).
- ✅ Graceful degradation: requesting CUDA on a CPU-only build logs and falls
  back rather than failing hard.
- ⚠️ TensorRT remains a build-gated stub; the factory returns a clear
  "not implemented" error when selected without the backend compiled in.
- ⚠️ The class-index → `ObjectClass` map is still set inside each engine's
  `initialize` (see ADR-0002); lifting it into config is deferred.
