# forklift-safety-system

> Real-time warehouse safety monitoring for RTSP cameras.
> Detects **PERSON** and **FORKLIFT**, draws safety zones, and emits
> WebSocket alerts when a person enters a zone. Built for edge boxes with
> 8–16 cameras.

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)]()
[![CMake](https://img.shields.io/badge/CMake-3.18%2B-green.svg)]()
[![License](https://img.shields.io/badge/license-Apache--2.0-lightgrey.svg)]()

---

## Table of contents

1. [Overview](#1-overview)
2. [Architecture](#2-architecture)
3. [Setup & build](#3-setup--build)
4. [Running & the dashboard](#4-running--the-dashboard)
5. [Configuration](#5-configuration)
6. [Deployment](#6-deployment)
7. [Development](#7-development)
8. [Coding conventions](#8-coding-conventions)
9. [Performance](#9-performance)
10. [Troubleshooting](#10-troubleshooting)
11. [License](#11-license)

---

## 1. Overview

`forklift-safety-system` is a single-process, multi-threaded C++17 service
that:

- Connects to 8–16 RTSP cameras simultaneously
- Runs a **YOLOv8** detector per frame (ONNX Runtime or TensorRT)
- Splits detections into `persons[]` and `forklifts[]`
- Expands each forklift's bounding box into a configurable **safety zone**
- Emits a `person_near_forklift` alert over **WebSocket** when a person
  overlaps any safety zone — with per-(camera, forklift) cooldown to
  prevent alert floods

It is designed for **edge deployment** (e.g. Jetson Nano): no cloud round-trip,
no database, no per-pixel egress. Outputs are alert JSON over WebSocket, an
optional **annotated MJPEG dashboard** for operators, and logs.

**Why a separate AI Box?** A dedicated edge process keeps latency tight
(p95 ≤ 100 ms), avoids RTSP fan-out to the cloud, and isolates safety
logic from the rest of the WMS stack.

---

## 2. Architecture

The system follows **Clean Architecture** with strict layering. Full
diagrams: [`docs/architecture/architecture.md`](docs/architecture/architecture.md).

```
                 interface  (src/interface/main.cpp — composition root)
                      │
                      ▼
                 application  (use cases, ports)
                  │       ▲
                  ▼       │
              domain    infrastructure  (RTSP, ORT/TRT, WebSocket, YAML, Logger)
                  ▲
                  └── shared  (ThreadPool, ConcurrentQueue<T>, Result<T>)
```

| Layer            | Knows about                       | Owns                                                  |
|------------------|-----------------------------------|-------------------------------------------------------|
| `domain/`        | nothing third-party               | `BoundingBox`, `Detection`, `SafetyZone`, `Alert`, `Frame`, `ObjectClass` |
| `application/`   | `domain/`, `shared/`              | `FrameProcessingPipeline`, `RiskDetectionService`, `SafetyZoneService`; ports: `InferenceEngine`, `VideoSource`, `AlertPublisher` |
| `infrastructure/`| `application/` ports              | `RtspCameraSource`, `OnnxInferenceEngine`, `TensorRTInferenceEngine`, `YoloV8Postprocessor`, `WebSocketAlertPublisher`, `YamlConfigLoader`, `Logger` |
| `interface/`     | everything                        | `main.cpp` only — wires components, handles signals  |
| `shared/`        | std only                          | `ThreadPool`, `ConcurrentQueue<T>`, `Result<T>`      |

Per-frame data flow (full version:
[`docs/architecture/pipeline.md`](docs/architecture/pipeline.md)):

```
RTSP → Capture thread → ConcurrentQueue<Frame> (drop-oldest)
     → ThreadPool worker
         ↳ InferenceEngine.infer()
         ↳ YoloV8Postprocessor (NMS, class map)
         ↳ RiskDetectionService (split, build zones, overlap test)
         ↳ Cooldown filter (per camera × forklift_track)
     → AlertPublisher (WebSocket fan-out)
```

Threading: one capture thread + a tiny `ThreadPool` of inference workers
per camera; one shared WebSocket publisher; deterministic shutdown.
Details: [`docs/architecture/threading.md`](docs/architecture/threading.md).

Key decisions:

- [ADR-0001](docs/adr/0001-clean-architecture.md) — Clean Architecture
- [ADR-0002](docs/adr/0002-inference-backends.md) — ORT + TensorRT side-by-side
- [ADR-0003](docs/adr/0003-frame-drop-oldest.md) — Drop-oldest queue policy
- [ADR-0004](docs/adr/0004-mjpeg-viewer.md) — Annotated MJPEG viewer
- [ADR-0005](docs/adr/0005-backend-device-selection.md) — Backend + device selection

For extending the AI or the UI, see
[`docs/architecture/ai-extensibility.md`](docs/architecture/ai-extensibility.md),
[`docs/architecture/ui-streaming.md`](docs/architecture/ui-streaming.md), and the
[production-readiness review](docs/architecture/production-readiness-review.md).

---

## 3. Setup & build

### Prerequisites

| Tool                | Version       | Required for             |
|---------------------|---------------|--------------------------|
| C++ compiler        | C++17         | Always                   |
| CMake               | ≥ 3.18        | Always                   |
| OpenCV              | ≥ 4.5 (FFmpeg)| Always                   |
| ONNX Runtime        | ≥ 1.13        | `FSS_WITH_ONNXRUNTIME=ON`|
| TensorRT            | ≥ 8.5         | `FSS_WITH_TENSORRT=ON`   |
| yaml-cpp            | ≥ 0.6         | optional — falls back to a stub loader |
| Boost (headers + system) | ≥ 1.74   | WebSocket alerts + MJPEG dashboard |
| Python + Ultralytics| 3.9 / latest  | model export only        |

#### macOS

```bash
brew install cmake opencv onnxruntime yaml-cpp boost
```

#### Ubuntu 22.04

```bash
sudo apt install -y build-essential cmake pkg-config \
                    libopencv-dev libyaml-cpp-dev libboost-system-dev
# Install ONNX Runtime from https://onnxruntime.ai/
```

### Build

```bash
git clone <this-repo>
cd forklift-safety-system

cmake -S . -B build \
      -DFSS_WITH_ONNXRUNTIME=ON \
      -DFSS_WITH_TENSORRT=OFF \
      -DFSS_BUILD_TESTS=ON
cmake --build build -j

# or
make            # default options
make test
make fmt
```

### Get a model

```bash
# Download the YOLOv8n person/forklift model
scripts/download_model.sh

# OR export your own custom-trained model
python scripts/export_yolov8.py --weights runs/train/exp/weights/best.pt \
                                --out models/yolov8_forklift.onnx
```

More: [`docs/development/local-setup.md`](docs/development/local-setup.md).

---

## 4. Running & the dashboard

Two modes, same annotated dashboard:

```bash
# Test — local video file (downloads model + sample video on first run)
make run-test
# → open http://127.0.0.1:8088/

# Production-style — live RTSP camera
make run-rtsp RTSP="rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"

# Or run a hand-written config directly
./build/forklift_safety --config conf/system.yaml
```

Every run brings up two servers:

| Service           | Default URL                    | What it is |
|-------------------|--------------------------------|------------|
| Dashboard (MJPEG) | `http://<host>:8088/`          | Live video with **person** (green), **forklift** (orange), and **safety-zone** (red) overlays, plus a live alert log |
| Alerts (WebSocket)| `ws://<host>:8765/ws/alerts`   | Alert JSON for integrations |

Tail alerts directly:

```bash
websocat ws://localhost:8765/ws/alerts
```

The dashboard is a static `web/` bundle (no build step) that embeds the MJPEG
stream and subscribes to the alert WebSocket. Design:
[`docs/architecture/ui-streaming.md`](docs/architecture/ui-streaming.md) and
[ADR-0004](docs/adr/0004-mjpeg-viewer.md).

---

## 5. Configuration

All runtime knobs live in [`conf/system.yaml`](conf/system.yaml) (RTSP) and
[`conf/system_test.yaml`](conf/system_test.yaml) (video file). Schema:

```yaml
backend: onnxruntime               # onnxruntime | tensorrt
log_level: info                    # debug | info | warn | error
inference_thread_pool_size: 0      # 0 ⇒ hardware_concurrency()

inference:
  model_path: models/yolov8n_forklift.onnx
  device: cpu                      # cpu | cuda  (cuda needs FSS_ORT_WITH_CUDA + CUDA ORT)
  input_width: 640
  input_height: 640
  conf_threshold: 0.35
  nms_threshold: 0.45
  max_detections: 300

safety_zone:
  mode: relative                   # relative | absolute
  pad_ratio: 0.5                   # mode=relative: fraction of box dim per side
  pad_x_px: 120                    # mode=absolute
  pad_y_px: 120

websocket:                         # alert JSON server
  host: 0.0.0.0
  port: 8765
  path: /ws/alerts

viewer:                            # annotated MJPEG dashboard (web/)
  enabled: true
  host: 0.0.0.0
  port: 8088
  web_root: web
  jpeg_quality: 75                 # 1..100
  target_fps: 15

cameras:
  - id: dock-01
    rtsp_url: "rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"
    # or, for Test mode:
    # source_type: video_file
    # video_path: test_data/forklift_test.mp4
    # loop: true
    queue_capacity: 4
    workers: 1
    alert_cooldown_ms: 2000
    enable_viewer: true            # per-camera viewer opt-in (annotation costs CPU)
```

Backend/device selection and how to inject a new AI:
[`docs/architecture/ai-extensibility.md`](docs/architecture/ai-extensibility.md).
Full field spec & validation rules:
[`docs/development/conventions/configuration.md`](docs/development/conventions/configuration.md).

---

## 6. Deployment

| Target                    | Command | Guide |
|---------------------------|---------|-------|
| Laptop — video file       | `make run-test` | [docs/deployment/laptop-testing.md](docs/deployment/laptop-testing.md) |
| Laptop — RTSP             | `make run-rtsp RTSP=...` | [docs/deployment/laptop-testing.md](docs/deployment/laptop-testing.md#run-against-a-live-rtsp-camera) |
| Jetson Nano — production  | `sudo ./deploy/install_jetson.sh [--cuda]` | [docs/deployment/jetson-nano.md](docs/deployment/jetson-nano.md) |

The Jetson installer builds, installs into `/opt/forklift-safety`, and enables a
**systemd service** that auto-starts on boot and restarts on failure. Overview:
[`docs/deployment/README.md`](docs/deployment/README.md).

---

## 7. Development

### Daily commands

```bash
make             # build
make test        # ctest
make fmt         # clang-format -i src/ include/ tests/
make clean
```

### Where to add things

| You are adding...                | Place it in...                                       |
|----------------------------------|------------------------------------------------------|
| A pure value type / rule         | `include/forklift/domain/`                           |
| A new use case / port            | `include/forklift/application/`                      |
| A new external adapter           | `src/infrastructure/<area>/`                         |
| Wiring                           | `src/interface/main.cpp`                             |
| A primitive (queue, pool, etc.)  | `include/forklift/shared/`                           |

End-to-end feature flow:
[`docs/development/implementing-new-feature.md`](docs/development/implementing-new-feature.md).

### OpenSpecs

The repo uses an **OpenSpec** workflow for documenting non-trivial
changes (briefs, proposals, tasks, ADRs). Start at
[`openspec/AUTHORING.md`](openspec/AUTHORING.md) and copy a template
from [`openspec/templates/`](openspec/templates).

Worked example:
[`openspec/examples/example-add-cooldown-policy/`](openspec/examples/example-add-cooldown-policy).

### Tests

Zero-dependency harness under `tests/`. Run:

```bash
cmake -S . -B build -DFSS_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

---

## 8. Coding conventions

Short version below; full set under
[`docs/development/conventions/`](docs/development/conventions).

- **Layering**: interface → application → domain; infrastructure
  implements application ports. No upward includes. No third-party
  includes inside `domain/`.
- **Naming**: types `PascalCase`, functions `snake_case`, members
  `snake_case_`, constants `kCamelCase`, header guards
  `FORKLIFT_<LAYER>_<NAME>_H_`. Files: `PascalCase.h` / `PascalCase.cpp`.
- **Error handling**: `shared::Result<T>` at boundaries; exceptions only
  for programmer errors; never swallow.
- **Threading**: no `detach`; one inference engine per worker; never hold
  a lock across inference; drop-oldest queue policy by default.
- **Logging**: `LOG_INFO/WARN/ERROR/DEBUG`; include `camera_id`; no PII.
- **Tests**: domain → 100 % branch coverage; application → fakes; no
  real RTSP / model / WS in unit tests.

---

## 9. Performance

| Metric                 | Target  |
|------------------------|---------|
| Frames per second      | ≥ 15 per camera |
| End-to-end latency p95 | ≤ 100 ms (capture → alert publish) |
| Cameras per box        | 8–16    |
| Inference batch size   | 1 (predictable latency) |

How we get there:

- **Bounded queues, drop-oldest** — see
  [ADR-0003](docs/adr/0003-frame-drop-oldest.md).
- **One inference engine per worker** — no shared mutable state on the
  hot path.
- **No hot-path allocation** — buffers reused; logging gated above DEBUG;
  no exceptions for normal flow.
- **Per-camera fault isolation** — one bad RTSP feed cannot stall the
  others.

Tune in `conf/system.yaml`:

```yaml
cameras:
  - queue_capacity: 4         # higher = more buffering, more latency
    workers: 1                # raise if your engine is thread-safe
    alert_cooldown_ms: 2000
```

Full notes:
[`docs/development/conventions/performance.md`](docs/development/conventions/performance.md).

---

## 10. Troubleshooting

| Symptom                                | Likely cause / fix                                         |
|----------------------------------------|------------------------------------------------------------|
| `RtspCameraSource[X]: failed to open`  | Bad URL or creds. Test with `ffprobe <rtsp_url>` first.    |
| Many `Dropped frame, queue full` warns | Inference is undersized. Raise `workers` or downgrade model (`yolov8n` instead of `yolov8s`). |
| No alerts despite people walking by    | Wrong class map. Confirm `0=person, 1=forklift` matches your trained model — see [`docs/development/conventions/inference.md`](docs/development/conventions/inference.md). |
| `cmake` cannot find ONNX Runtime       | Set `-DCMAKE_PREFIX_PATH=/path/to/onnxruntime` or build with `-DFSS_WITH_ONNXRUNTIME=OFF`. |
| WebSocket clients don't receive alerts | Confirm port is reachable; check `websocket.host` (use `0.0.0.0` for non-localhost clients). |
| Dashboard reachable but no video       | Set `viewer.enabled: true` and the camera's `enable_viewer: true`; the dashboard is port **8088**, alerts are **8765**. |
| Alert flood on a single incursion      | Confirm `alert_cooldown_ms` is set; default 2000 ms is per (camera, forklift_track). |
| Process never exits on Ctrl-C          | SIGINT handler missed; send `SIGTERM`. File an issue with the stack trace from `gdb -p <pid>`. |
| TSAN reports a data race               | Re-read [`docs/development/conventions/threading.md`](docs/development/conventions/threading.md). Almost always a mutex-less shared mutable. |

For anything not listed: open a GitHub issue with logs from
`./build/forklift_safety --config conf/system.yaml --log-level=debug`,
your `conf/system.yaml` (with credentials redacted), and `cmake --version`
/ `ldd ./build/forklift_safety`.

---

## 11. License

Apache-2.0. See `LICENSE`.

---

### Project map

```
forklift-safety-system/
├── AGENTS.md                    ← read me first if you're an AI assistant
├── README.md                    ← you are here
├── CMakeLists.txt
├── Makefile                     ← run-test · run-rtsp · deploy-jetson · …
├── conf/                        ← system.yaml (RTSP) · system_test.yaml (video)
├── models/                      ← ONNX artefacts
├── web/                         ← static dashboard (index.html · app.js · style.css)
├── scripts/                     ← setup_test · run_test · run_rtsp · download_model · export_yolov8
├── deploy/                      ← install_jetson.sh · forklift-safety.service (systemd)
├── docs/
│   ├── adr/                     ← ADR-0001 … 0005
│   ├── architecture/            ← architecture · pipeline · threading · websocket-api
│   │                              · ai-extensibility · ui-streaming · production-readiness-review
│   ├── deployment/              ← README · jetson-nano · laptop-testing
│   └── development/
│       ├── conventions/         ← cpp · naming · threading · error-handling · …
│       ├── local-setup.md
│       ├── implementing-new-feature.md
│       └── code-review.md
├── openspec/                    ← change workflow (README · AUTHORING · templates · examples)
├── include/forklift/{domain,application,infrastructure,shared}/
├── src/{domain,application,infrastructure,interface,shared}/
└── tests/{domain,application}/
```
