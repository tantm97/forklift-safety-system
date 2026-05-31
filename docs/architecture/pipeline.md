# Pipeline (per camera)

```
  RTSP url
     │
     ▼
┌──────────────────────┐
│ RtspCameraSource     │   capture thread (one per camera)
│ (cv::VideoCapture +  │
│  FFmpeg backend)     │
└──────────┬───────────┘
           │ domain::Frame
           ▼
┌──────────────────────┐
│ ConcurrentQueue<     │   capacity = 4 (configurable)
│   Frame >            │   policy   = DropOldestWhenFull (ADR-0003)
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ ThreadPool worker    │   N workers per camera (default 1)
│  ├─ InferenceEngine  │   ONNX Runtime or TensorRT (ADR-0002)
│  ├─ YoloV8           │   tensor decode + NMS
│  │  Postprocessor    │
│  ├─ RiskDetection    │   split persons/forklifts; build SafetyZones;
│  │  Service          │   overlap test
│  └─ cooldown filter  │   per-(camera, forklift_track) suppression
└──────────┬───────────┘
           │ domain::Alert
           ▼
┌──────────────────────┐
│ AlertPublisher       │   shared across all cameras
│ (WebSocket fan-out)  │
└──────────────────────┘
```

## Stages

| Stage | Owner | Threading | Notes |
|-------|-------|-----------|-------|
| Capture           | `RtspCameraSource`             | one dedicated thread per camera   | OpenCV buffer size forced to 1 to minimise queueing latency |
| Queue             | `ConcurrentQueue<Frame>`       | n/a                               | Drop-oldest by default |
| Preprocess        | `OnnxInferenceEngine::infer`   | inference worker                  | letterbox / resize / blobFromImage |
| Inference         | `OnnxInferenceEngine::infer`   | inference worker                  | Single-batch (batch=1) for predictable latency |
| Postprocess (NMS) | `YoloV8Postprocessor`          | inference worker                  | cv::dnn::NMSBoxes |
| Risk evaluation   | `RiskDetectionService`         | inference worker                  | Pure domain logic |
| Cooldown          | `FrameProcessingPipeline`      | inference worker                  | unordered_map<forklift_track, last_ts> |
| Publish           | `WebSocketAlertPublisher`      | publisher thread (WS lib)         | Serialises Alert → JSON; fans out to clients |

## Cooldown semantics

When the same `forklift_track_id` triggers an alert within
`alert_cooldown_ms` of the previous one, the new alert is suppressed. The
counter resets the moment cooldown elapses; the first overlapping frame
after the window emits a fresh alert.

## Latency budget (per frame)

| Budget item            | Target (ms) |
|------------------------|-------------|
| Capture → queue        | < 5  |
| Queue wait             | < 30 |
| Preprocess             | < 10 |
| Inference (yolov8n CPU)| < 40 |
| Postprocess + risk     | < 5  |
| Publish                | < 5  |
| **End-to-end p95**     | **≤ 100** |

A 15 FPS budget = 66 ms per frame; we accept queue drops above this rate.

## Failure modes

| Symptom                       | Mitigation |
|-------------------------------|-----------|
| RTSP disconnect               | Capture loop reopens with exponential backoff up to `max_reconnect_backoff_ms` |
| Inference exception           | Logged at WARN; frame dropped; next frame proceeds |
| WebSocket clients all dropped | Publisher logs at WARN; alerts still serialised for the log sink |
| Queue overflow                | Oldest frame dropped; `ConcurrentQueue::dropped()` increments |
