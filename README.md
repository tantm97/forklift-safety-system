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
4. [Configuration](#4-configuration)
5. [Development](#5-development)
6. [Coding conventions](#6-coding-conventions)
7. [Performance](#7-performance)
8. [Troubleshooting](#8-troubleshooting)
9. [License](#9-license)

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

It is designed for **edge deployment**: no cloud round-trip, no database,
no per-pixel egress. The only outputs are alert JSON messages and logs.

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
| Python + Ultralytics| 3.9 / latest  | model export only        |

#### macOS

```bash
brew install cmake opencv onnxruntime yaml-cpp
```

#### Ubuntu 22.04

```bash
sudo apt install -y build-essential cmake pkg-config \
                    libopencv-dev libyaml-cpp-dev
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
# Pretrained YOLOv8n (person only)
scripts/download_model.sh

# OR export your own custom-trained model
python scripts/export_yolov8.py --weights runs/train/exp/weights/best.pt \
                                --out models/yolov8_forklift.onnx
```

### Run

```bash
./build/forklift_safety --config conf/system.yaml
```

Tail alerts in another shell:

```bash
websocat ws://localhost:8765/ws/alerts
```

More: [`docs/development/local-setup.md`](docs/development/local-setup.md).

---

## 4. Configuration

All runtime knobs live in [`conf/system.yaml`](conf/system.yaml). Schema:

```yaml
inference:
  backend: onnxruntime            # onnxruntime | tensorrt
  model_path: models/yolov8n.onnx
  confidence_threshold: 0.35
  iou_threshold: 0.45
  input_size: 640

safety_zone:
  expand_ratio: 1.5               # zone = forklift_box × expand_ratio
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

Full spec & validation rules:
[`docs/development/conventions/configuration.md`](docs/development/conventions/configuration.md).

---

## 5. Development

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

## 6. Coding conventions

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

## 7. Performance

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
    inference_workers: 1      # raise if your engine is thread-safe
    alert_cooldown_ms: 2000
```

Full notes:
[`docs/development/conventions/performance.md`](docs/development/conventions/performance.md).

---

## 8. Troubleshooting

| Symptom                                | Likely cause / fix                                         |
|----------------------------------------|------------------------------------------------------------|
| `RtspCameraSource[X]: failed to open`  | Bad URL or creds. Test with `ffprobe <rtsp_url>` first.    |
| Many `Dropped frame, queue full` warns | Inference is undersized. Raise `inference_workers` or downgrade model (`yolov8n` instead of `yolov8s`). |
| No alerts despite people walking by    | Wrong class map. Confirm `0=person, 1=forklift` matches your trained model — see [`docs/development/conventions/inference.md`](docs/development/conventions/inference.md). |
| `cmake` cannot find ONNX Runtime       | Set `-DCMAKE_PREFIX_PATH=/path/to/onnxruntime` or build with `-DFSS_WITH_ONNXRUNTIME=OFF`. |
| WebSocket clients don't receive alerts | Confirm port is reachable; check `websocket.host` (use `0.0.0.0` for non-localhost clients). |
| Alert flood on a single incursion      | Confirm `alert_cooldown_ms` is set; default 2000 ms is per (camera, forklift_track). |
| Process never exits on Ctrl-C          | SIGINT handler missed; send `SIGTERM`. File an issue with the stack trace from `gdb -p <pid>`. |
| TSAN reports a data race               | Re-read [`docs/development/conventions/threading.md`](docs/development/conventions/threading.md). Almost always a mutex-less shared mutable. |

For anything not listed: open a GitHub issue with logs from
`./build/forklift_safety --config conf/system.yaml --log-level=debug`,
your `conf/system.yaml` (with credentials redacted), and `cmake --version`
/ `ldd ./build/forklift_safety`.

---

## 9. License

Apache-2.0. See `LICENSE`.

---

### Project map

```
forklift-safety-system/
├── AGENTS.md                    ← read me first if you're an AI assistant
├── README.md                    ← you are here
├── CMakeLists.txt
├── Makefile
├── conf/system.yaml             ← runtime config
├── models/                      ← ONNX artefacts
├── scripts/                     ← export_yolov8.py · download_model.sh
├── docs/
│   ├── adr/                     ← ADR-0001 … 0003
│   ├── architecture/            ← architecture · pipeline · threading · websocket-api
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
