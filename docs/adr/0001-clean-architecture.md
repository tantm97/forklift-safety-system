# ADR-0001: Adopt Clean Architecture in five layers

**Date:**     2026-05-23
**Status:**   Accepted
**Category:** Architecture
**Change:**   bootstrap

## Context

The system must support 8–16 cameras concurrently, swap between ONNX Runtime
and TensorRT inference backends per deployment, and be testable without
requiring a real RTSP source, real model, or real WebSocket client.

Mixing OpenCV/ORT types into business logic locks us into those dependencies
and makes unit testing impractical. We need explicit boundaries.

## Decision

Five strict layers:

```
interface (entrypoint)
    │
    ▼
application (use cases, ports)
    │             ▲
    ▼             │
domain   ◄──── infrastructure (adapters: RTSP, ORT, TRT, WS, YAML, log)
    ▲
    └──── shared (ThreadPool, ConcurrentQueue, Result<T>)
```

- `domain/` has ZERO third-party includes. Pure value types.
- `application/` defines ports (`InferenceEngine`, `VideoSource`,
  `AlertPublisher`) and use cases. Depends only on `domain` and `shared`.
- `infrastructure/` implements ports. It is the only layer allowed to
  include OpenCV, ONNX Runtime, TensorRT, networking headers.
- `interface/` is the composition root (`src/interface/main.cpp`).
- `shared/` is leaf-level primitives used by any layer.

Enforced by convention + code review. CMake puts headers under
`include/forklift/<layer>/` so dependency direction is visible in includes.

## Consequences

- ✅ Domain tests run in <50 ms with no external deps.
- ✅ Backend swap (ORT ↔ TRT) is a one-line change in `main.cpp`.
- ✅ Mock implementations for `VideoSource` enable deterministic integration
  tests.
- ⚠️ One extra indirection (port → impl) per pipeline stage — measured
  overhead is < 1 % of inference cost; acceptable.
- ⚠️ More files. Mitigated by the consistent `<layer>/<ClassName>.{h,cpp}`
  layout.
