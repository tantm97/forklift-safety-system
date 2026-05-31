# AGENTS.md — forklift-safety-system

Read this before generating or reviewing any change in this repo.

## OVERVIEW

Single-process, multi-threaded C++17 edge service that monitors 8–16 RTSP
camera streams, runs a YOLOv8 detector (ONNX Runtime or TensorRT), defines
a safety zone around each forklift, and emits WebSocket alerts when a
person enters a zone.

Build: CMake ≥ 3.18. Style: clang-format (Google-ish, see
`docs/development/conventions/cpp.md`). Test: `ctest`.

## STRUCTURE

```
include/forklift/<layer>/   public headers (domain | application | infrastructure | shared)
src/<layer>/                implementations + interface/main.cpp (composition root)
docs/                       adr/, architecture/, development/conventions/
openspec/                   change workflow (see openspec/AGENTS.md)
conf/                       runtime config (system.yaml)
models/                     ONNX artefacts + class-name lists
scripts/                    export_yolov8.py, download_model.sh
tests/                      domain/ + application/, zero-dep harness
```

## WHERE TO LOOK

| Question                           | File                                                                |
|------------------------------------|---------------------------------------------------------------------|
| How does a frame flow?             | `docs/architecture/pipeline.md`                                     |
| What runs on which thread?         | `docs/architecture/threading.md`                                    |
| What layer may include what?       | `docs/architecture/module-boundaries.md`                            |
| What's the alert JSON?             | `docs/architecture/websocket-api.md`                                |
| Why this architecture?             | `docs/adr/0001-clean-architecture.md`                               |
| Where do I add a new feature?      | `docs/development/implementing-new-feature.md`                      |
| How do I author a change?          | `openspec/AUTHORING.md`                                             |
| Where is the composition root?     | `src/interface/main.cpp`                                            |
| Where is the pipeline orchestrator?| `src/application/FrameProcessingPipeline.cpp`                       |
| Where is the safety logic?         | `src/application/RiskDetectionService.cpp`                          |

## CONVENTIONS

- **Layering** (`docs/development/conventions/architecture.md`):
  interface → application → domain; infrastructure implements application
  ports; no upward includes; no third-party deps in `domain/`.
- **Naming** (`naming.md`): types PascalCase, functions snake_case,
  members snake_case_, enum values `kCamelCase`, header guards
  `FORKLIFT_<LAYER>_<NAME>_H_`.
- **Error handling** (`error-handling.md`): `shared::Result<T>` at
  boundaries; exceptions only for programmer errors.
- **Threading** (`threading.md`): no detached threads; one inference
  engine per worker; never hold a lock across inference; drop-oldest on
  bounded queues by default.
- **Performance** (`performance.md`): no allocation / formatting /
  exceptions on the per-frame hot path; ≥ 15 FPS per camera; p95 ≤ 100 ms.
- **Logging** (`logging.md`): `LOG_INFO/WARN/ERROR/DEBUG` macros; include
  `camera_id`; no PII; no RTSP credentials.
- **Testing** (`testing.md`): domain → 100 % branch coverage; application
  → fakes, never real model/RTSP/WS.

## GOTCHAS

- `cv::Mat` is shallow-copied. `std::move` it across thread boundaries
  anyway — it documents intent and avoids accidental sharing of the same
  underlying buffer after future refactors.
- `OnnxInferenceEngine` is **not thread-safe**. One instance per
  inference worker.
- The class map (`0=person, 1=forklift`) is hard-coded in
  `OnnxInferenceEngine::initialize`. Re-trained models with a different
  index order will silently produce wrong alerts. File a change to lift
  this into config when you touch it.
- The WebSocket publisher today is a stdio-stub (logs JSON). The real WS
  library is intentionally swappable behind the `AlertPublisher` port.
- `FSS_HAS_YAML_CPP` propagates via PUBLIC compile definitions — do NOT
  `#define` it in source files.

## LEARNINGS

- Per-camera independence (one capture + one tiny ThreadPool) beats a
  shared inference pool for fault isolation in field deployments.
- Cooldown belongs in `FrameProcessingPipeline` (close to the pipeline
  state), not in `RiskDetectionService` (which must stay pure).
- Adding a port-and-fake pair before the real adapter pays back within
  one PR cycle — every time.
