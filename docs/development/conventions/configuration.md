# Configuration

Single YAML config: `conf/system.yaml`. Loaded once at startup by
`infrastructure::config::YamlConfigLoader`.

## Top-level shape

```yaml
inference:
  backend: onnxruntime            # onnxruntime | tensorrt
  model_path: models/yolov8n.onnx
  confidence_threshold: 0.35
  iou_threshold: 0.45
  input_size: 640

safety_zone:
  expand_ratio: 1.5               # safety_zone = forklift_box × expand_ratio
  min_zone_padding_px: 40

websocket:
  host: 0.0.0.0
  port: 8765
  path: /ws/alerts

cameras:
  - id: dock-01
    rtsp_url: rtsp://user:pass@10.0.0.10/Streaming/Channels/101
    queue_capacity: 4
    inference_workers: 1
    alert_cooldown_ms: 2000
    max_reconnect_backoff_ms: 30000
  - id: dock-02
    rtsp_url: rtsp://user:pass@10.0.0.11/Streaming/Channels/101
```

## Rules

- All keys are `snake_case`.
- All durations end in `_ms` / `_us`.
- All distances in pixels end in `_px`; ratios are unit-less.
- Defaults live in `YamlConfigLoader` so a minimal config still works.
- Sensitive values (RTSP credentials) may be injected via env vars in a
  future change; today they live in the YAML file with `chmod 600`.

## Validation

- Empty `cameras` list → fail fast.
- Duplicate `cameras[].id` → fail fast.
- `inference.backend: tensorrt` but binary built without `FSS_WITH_TENSORRT`
  → fail fast with a clear message.
- Missing model file → fail fast.

## Reload

The system does **not** support hot-reload. Changes require a restart.
Documented trade-off: simpler concurrency model, no consistency window.
