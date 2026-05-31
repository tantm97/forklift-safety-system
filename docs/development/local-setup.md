# Local Setup

## Prerequisites

- C++17 compiler (g++ ≥ 9, clang ≥ 11, AppleClang ≥ 12)
- CMake ≥ 3.18
- OpenCV ≥ 4.5 (with FFmpeg for RTSP)
- (optional) ONNX Runtime ≥ 1.13
- (optional) TensorRT ≥ 8.5 (NVIDIA targets only)
- (optional) yaml-cpp ≥ 0.6 — falls back to a built-in stub loader
- Python ≥ 3.9 + Ultralytics (only for model export)

## macOS (Homebrew)

```bash
brew install cmake opencv onnxruntime yaml-cpp
```

## Ubuntu 22.04

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
                    libopencv-dev libyaml-cpp-dev
# ONNX Runtime: download release tarball or build from source
# https://onnxruntime.ai/
```

## Build

```bash
git clone https://github.com/<you>/forklift-safety-system
cd forklift-safety-system

cmake -S . -B build \
      -DFSS_WITH_ONNXRUNTIME=ON \
      -DFSS_WITH_TENSORRT=OFF \
      -DFSS_BUILD_TESTS=ON
cmake --build build -j
```

Or use the Makefile shortcuts:

```bash
make            # configure + build (default options)
make test       # build + run ctest
make fmt        # clang-format -i over src/ include/ tests/
make clean
```

## Run

```bash
# 1. Get a model
scripts/download_model.sh           # or scripts/export_yolov8.py
ls models/yolov8n.onnx

# 2. Edit conf/system.yaml — set your RTSP URLs

# 3. Launch
./build/forklift_safety --config conf/system.yaml
```

## Smoke test

In another terminal:

```bash
# Tail the alert stream (any WS client works)
websocat ws://localhost:8765/ws/alerts
```

Walk past a forklift in view of a configured camera — you should see
JSON alerts arrive.
