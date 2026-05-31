---
applyTo: '**/infrastructure/ai/**/*.{h,cpp}'
description: 'Inference adapter conventions (ONNX Runtime / TensorRT)'
---

> **Source of truth:** [`docs/development/conventions/inference.md`](../../docs/development/conventions/inference.md)

Read the full inference document before editing inference adapters.

Key rules (quick reference):
- All inference adapters implement `application::InferenceEngine` (`initialize`, `infer`, `backend_name`).
- One `OrtSession` (or TRT engine) per `InferenceEngine` instance — **not thread-safe**. Use one instance per worker thread.
- Never share a session across threads. Use `ThreadPool` with one engine per worker.
- YOLOv8 output layout: `[1, 4 + num_classes, num_anchors]` — channels `cx, cy, w, h, cls0…clsN-1`.
- Postprocessing (letterbox → blob → NMS) lives in `YoloV8Postprocessor`, not inside the engine.
- Class map (`0=person, 1=forklift`) is hard-coded in `OnnxInferenceEngine::initialize`. Tag any change to the map with a follow-up brief to move it to config.
- New backends: add `FSS_WITH_<BACKEND>` CMake option; reuse `YoloV8Postprocessor`; do not duplicate NMS.
- PIMPL for heavy third-party includes (ORT, TRT headers stay in `.cpp`, not in the public header).
