# Architecture Overview

`forklift-safety-system` is a single-process, multi-threaded C++17 edge
application that monitors 8–16 RTSP camera streams in real time, detects
PERSON and FORKLIFT objects with a YOLOv8 model, defines a safety zone
around each forklift, and emits a WebSocket alert when a person enters a
zone.

## Layered architecture (see [ADR-0001](../adr/0001-clean-architecture.md))

```
                ┌───────────────────────────────────────────────┐
                │                  interface                    │
                │            (src/interface/main.cpp)           │
                │  composition root · CLI · signals             │
                └────────────────────┬──────────────────────────┘
                                     │
                                     ▼
                ┌───────────────────────────────────────────────┐
                │                application                    │
                │  FrameProcessingPipeline                      │
                │  SafetyZoneService · RiskDetectionService     │
                │  ports: InferenceEngine · VideoSource ·       │
                │         AlertPublisher                        │
                └────────────────────┬──────────────────────────┘
                                     │
            ┌────────────────────────┴────────────────────────┐
            ▼                                                 ▼
┌─────────────────────────┐                ┌─────────────────────────────┐
│        domain           │                │       infrastructure         │
│  Frame · Detection ·    │ ◄──────────── │  RtspCameraSource            │
│  BoundingBox ·          │   adapters     │  OnnxInferenceEngine         │
│  SafetyZone · Alert ·   │   implement    │  TensorRTInferenceEngine     │
│  ObjectClass            │   ports        │  YoloV8Postprocessor         │
│  (no third-party deps)  │                │  WebSocketAlertPublisher     │
└─────────────────────────┘                │  YamlConfigLoader · Logger   │
                                           └─────────────────────────────┘
                ┌───────────────────────────────────────────────┐
                │                   shared                      │
                │  ThreadPool · ConcurrentQueue<T> · Result<T>  │
                └───────────────────────────────────────────────┘
```

## Why this shape

- **Domain isolation.** The hardest-to-test types (BoundingBox geometry,
  zone overlap, alert construction) have no external dependencies and run
  in microsecond unit tests.
- **Pluggable inference.** `application::InferenceEngine` decouples the
  pipeline from the runtime. ORT today, TensorRT for NVIDIA edge, OpenVINO
  tomorrow — see [ADR-0002](../adr/0002-inference-backends.md).
- **Pluggable transport.** `application::AlertPublisher` lets us swap the
  WebSocket sink for MQTT, Kafka, or a stdout fake in tests.
- **Composition root is dumb.** `main.cpp` only wires; it has no
  if/else over object classes, models, or zones.

## Multi-camera scaling

Each camera is a fully independent `FrameProcessingPipeline`:

- One capture thread (owns `cv::VideoCapture`)
- One bounded `ConcurrentQueue<Frame>` (capacity 4, drop-oldest)
- A small `ThreadPool` (default 1 worker) of inference workers
- Shared singletons across cameras: `WebSocketAlertPublisher`,
  `SafetyZoneService`, `RiskDetectionService`

This gives us camera-level fault isolation (one bad RTSP feed can't stall
the others) and trivial horizontal scaling at the process level (split
cameras across N processes if a single GPU saturates).

## See also

- [pipeline.md](pipeline.md)        — frame-level data flow & stages
- [threading.md](threading.md)      — thread topology & ownership rules
- [module-boundaries.md](module-boundaries.md) — what each layer may import
- [websocket-api.md](websocket-api.md) — JSON alert schema
