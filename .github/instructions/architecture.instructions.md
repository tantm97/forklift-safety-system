---
applyTo: '**/*.{h,cpp}'
description: 'Layered architecture rules for forklift-safety-system'
---

> **Source of truth:** [`docs/development/conventions/architecture.md`](../../docs/development/conventions/architecture.md)

Read the full architecture document before generating or editing C++ files.

Key rules (quick reference):
- Layer flow: `interface → application → domain`; infrastructure implements application ports. Calls flow downward only.
- `domain/` headers have ZERO third-party includes. Only `<std*>`, `<chrono>`, `<cstdint>`, `<string>`, and `cv::Mat` (inside `Frame.h` only).
- `application/` defines ports (`InferenceEngine`, `VideoSource`, `AlertPublisher`) and use cases. Never includes infrastructure headers.
- `infrastructure/` is the only layer allowed to include OpenCV, ONNX Runtime, TensorRT, networking, yaml-cpp.
- `interface/main.cpp` is the composition root — wires components, handles signals, contains no business logic.
- `shared/` (ThreadPool, ConcurrentQueue, Result<T>) is leaf-level; no other layer imports.
- Include path audit: `! grep -r 'infrastructure' include/forklift/domain/ include/forklift/application/`
- Canonical reference: `src/application/RiskDetectionService.cpp` (pure domain logic, no third-party deps).
