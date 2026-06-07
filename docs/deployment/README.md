# Deployment

How to run `forklift-safety-system` in each environment.

| Environment            | Input        | Guide |
|------------------------|--------------|-------|
| Laptop / dev — Test    | video file   | [laptop-testing.md](laptop-testing.md) |
| Laptop / dev — RTSP    | RTSP camera  | [laptop-testing.md](laptop-testing.md#run-against-a-live-rtsp-camera) |
| Jetson Nano — Production| RTSP cameras | [jetson-nano.md](jetson-nano.md) |

## At a glance

Both servers come up on every run:

| Service          | Default URL                         | Purpose |
|------------------|-------------------------------------|---------|
| Dashboard (MJPEG)| `http://<host>:8088/`               | Live annotated video + alert log |
| Alerts (WS)      | `ws://<host>:8765/ws/alerts`        | Alert JSON for integrations |

## Quick start

```bash
# Laptop, video file (downloads model + test video on first run)
make run-test
# → open http://127.0.0.1:8088/

# Laptop, live RTSP camera
make run-rtsp RTSP="rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"

# Jetson Nano, install + auto-start on boot
sudo ./deploy/install_jetson.sh            # add --cuda for the GPU build
```

## Build flags that matter for deployment

| Flag                      | Default | Effect |
|---------------------------|---------|--------|
| `FSS_WITH_ONNXRUNTIME`    | ON      | ONNX Runtime backend |
| `FSS_WITH_TENSORRT`       | OFF     | TensorRT backend (Jetson; currently a stub) |
| `FSS_ORT_WITH_CUDA`       | OFF     | Compile the CUDA execution provider for ORT |
| `FSS_BUILD_TESTS`         | ON      | Unit tests (OFF for installs) |

Backend and device are then chosen at **runtime** in config
(`inference.backend`, `inference.device`) — see
[ADR-0005](../adr/0005-backend-device-selection.md) and
[ai-extensibility](../architecture/ai-extensibility.md).

## Configuration reference

The full annotated schema lives in the two reference configs:

- [`conf/system_test.yaml`](../../conf/system_test.yaml) — video-file Test mode
- [`conf/system.yaml`](../../conf/system.yaml) — RTSP Production mode

Field rules: [`../development/conventions/configuration.md`](../development/conventions/configuration.md).
