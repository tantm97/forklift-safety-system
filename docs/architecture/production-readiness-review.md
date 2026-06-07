# Production-readiness review — Jetson Nano

A review of `forklift-safety-system` against the goals: production-ready on
**Jetson Nano**, easy to maintain, **AI separated from the stream**, and easy to
**change the model / change the AI method / inject new AI** later.

Date: 2026-06-01 · Reviewer: engineering.

## Verdict

The architecture is sound for the stated goals. Clean layering is real, not
aspirational: the pipeline depends only on ports, so the AI and the stream are
genuinely decoupled and independently swappable. The gaps that remained
(frame/detection streaming, device selection, a documented AI extension point,
deploy automation) are addressed by the changes recorded in
[ADR-0004](../adr/0004-mjpeg-viewer.md) and
[ADR-0005](../adr/0005-backend-device-selection.md). Remaining items below are
**operational hardening**, not architectural blockers.

## Scorecard

| Dimension                        | Status | Notes |
|----------------------------------|:------:|-------|
| AI separated from stream         |   ✅   | `VideoSource` and `InferenceEngine` are independent ports; pipeline names neither concretely. |
| Change the model                 |   ✅   | Config-only (`inference.model_path`). Caveat: class-map is in-engine. |
| Change the device (CPU/CUDA)     |   ✅   | `inference.device`; CUDA via `-DFSS_ORT_WITH_CUDA`, CPU fallback w/ warning. |
| Inject a new AI method           |   ✅   | `InferenceEngineFactory` is the single extension point — see [ai-extensibility](ai-extensibility.md). |
| Operator UI (Test + Production)  |   ✅   | Annotated MJPEG dashboard, identical for video and RTSP. |
| Deploy + auto-start on Jetson    |   ✅   | systemd unit + `deploy/install_jetson.sh`; laptop scripts for video/RTSP. |
| Fault isolation (bad camera)     |   ✅   | Per-camera capture + tiny pool; one feed cannot stall others (ADR-0003). |
| Graceful shutdown                |   ✅   | SIGINT/SIGTERM → deterministic stop of pipeline + both servers. |
| Test coverage                    |   🟡   | Domain + application covered with fakes; adapters (ORT/RTSP/WS/MJPEG) are integration-tested manually. |
| Secrets handling                 |   🟡   | RTSP creds live in `system.yaml`; not logged (good), but file is plaintext. |
| Observability / metrics          |   🟡   | Structured logs only; no FPS/latency/queue-depth metrics endpoint yet. |
| Resource limits / watchdog       |   🟡   | systemd `Restart=on-failure`; no per-camera FPS watchdog or memory cap. |

## Strengths

- **Ports & adapters are honest.** `application/` has no third-party includes;
  OpenCV, ONNX Runtime, and Boost.Beast live only in `infrastructure/`. This is
  what makes "swap the AI" and "swap the transport" cheap.
- **The hot path is protected.** Annotation works on a frame clone; the viewer
  is opt-in per camera; drop-oldest queues bound latency (ADR-0003).
- **One binary, many targets.** Build flags (`FSS_WITH_ONNXRUNTIME`,
  `FSS_WITH_TENSORRT`, `FSS_ORT_WITH_CUDA`) + config (`backend`, `device`) cover
  CPU laptops and CUDA Jetsons without code forks.
- **Deterministic shutdown** and per-camera fault isolation suit unattended
  field boxes.

## Jetson Nano specifics

- Build the installer with `--cuda` to compile the CUDA EP
  (`deploy/install_jetson.sh --cuda`). Requires a CUDA-enabled ONNX Runtime in
  the JetPack image.
- The Nano is memory-constrained: prefer `yolov8n`, keep `queue_capacity`
  small (4), one `workers` per camera, and enable the viewer on only the cameras
  actually watched.
- For best latency on Jetson, TensorRT is the long-term target; it is currently
  a build-gated stub (see ADR-0002). The factory already accommodates it.
- Pin the model and config under `/opt/forklift-safety`; the systemd unit sets
  `WorkingDirectory` so relative paths in the config resolve.

## Recommended next hardening steps (not blockers)

1. **Lift the class map into config** — remove the silent-wrong-alert risk when
   models are retrained with a different class order (ADR-0002 caveat).
2. **Metrics endpoint** — expose per-camera FPS, queue depth, inference p95, and
   reconnect counts (Prometheus text or a `/metrics` JSON) for field monitoring.
3. **Per-camera watchdog** — restart a capture that has produced no frames for N
   seconds, independently of the systemd process-level restart.
4. **Secrets** — move RTSP credentials out of plaintext YAML (env var
   interpolation or a secrets file with `0600`); the loader already avoids
   logging them.
5. **TensorRT adapter** — implement the stub for production Jetson latency.
6. **Adapter integration tests in CI** — a recorded short video + a tiny ONNX
   model to exercise `OnnxInferenceEngine` and the MJPEG server end-to-end.

## How to re-run this review

- Architecture: [`architecture.md`](architecture.md),
  [`module-boundaries.md`](module-boundaries.md),
  [`threading.md`](threading.md).
- Extensibility: [`ai-extensibility.md`](ai-extensibility.md),
  [`ui-streaming.md`](ui-streaming.md).
- Deploy: [`../deployment/README.md`](../deployment/README.md).
