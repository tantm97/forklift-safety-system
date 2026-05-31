# Design: {{change-name}}

## Architecture Overview

<!-- High-level. Which layer(s) change. How it fits the
     interface → application → domain ← infrastructure rule. -->

## Technical Decisions

### Chosen Approach

### Alternatives Considered

| Alternative | Pros | Cons | Why Rejected |
|-------------|------|------|--------------|
|             |      |      |              |

## Component Design

### New Components

- `include/forklift/<layer>/<Name>.h` + `src/<layer>/<Name>.cpp`

### Modified Components

## Data Flow

```
RTSP → cv::VideoCapture → ConcurrentQueue<Frame>
     → ThreadPool worker
        → InferenceEngine.infer()
        → YoloV8Postprocessor (NMS)
        → RiskDetectionService.evaluate()
        → AlertPublisher.publish()
              → WebSocket clients
```

## Threading Model

<!-- Which threads run; which queues separate them; ownership lifetimes;
     re-entrancy guarantees. -->

## Configuration & Migration

### New keys

### Default behaviour

### Backwards-compat plan

## Observability

<!-- Log lines (LOG_INFO/WARN/ERROR), counters, exposed metrics. -->

## Security Considerations

<!-- Credentials in conf/system.yaml, WS auth, model file integrity. -->

## Performance Considerations

- FPS budget per camera
- Memory per pipeline instance
- GPU utilisation (if applicable)

## Testing Strategy

- Unit tests:        `tests/<layer>/<name>_test.cpp`
- Integration test:  multi-camera smoke run with synthetic frames
- Manual smoke:      `./build/forklift_safety --config conf/system.yaml`

## Deployment Considerations

<!-- ONNX model placement, CMake options, runtime env vars, systemd unit. -->

## Dependencies

### Upstream

### Downstream
