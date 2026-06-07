# Laptop testing (Test + RTSP modes)

Run the detector on a development laptop — either against a downloaded **video
file** (no camera needed) or a **live RTSP** camera. Both modes serve the same
annotated dashboard.

## Prerequisites

```bash
# macOS
brew install cmake opencv onnxruntime yaml-cpp boost

# Ubuntu 22.04
sudo apt install -y build-essential cmake pkg-config \
     libopencv-dev libyaml-cpp-dev libboost-system-dev
# + install ONNX Runtime from https://onnxruntime.ai/
```

## Test mode — video file

One command builds, downloads the model + a sample forklift/person video, and
starts the system:

```bash
make run-test
# equivalently: ./scripts/run_test.sh
```

Then open the dashboard:

- Dashboard: <http://127.0.0.1:8088/>
- Alerts:    `ws://127.0.0.1:8765/ws/alerts`

What `run_test.sh` does:

1. `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j`
2. `scripts/setup_test.sh` — downloads `models/yolov8n_forklift.onnx` and
   `test_data/forklift_test.mp4` if missing
3. runs `./build/forklift_safety --config conf/system_test.yaml`

The test config (`conf/system_test.yaml`) uses a single `video_file` camera that
loops, binds servers to `127.0.0.1`, and enables the viewer. To build with the
CUDA EP for a quick GPU smoke test: `FSS_CUDA=1 make run-test` (needs a
CUDA-enabled ORT).

Tail alerts in another shell instead of (or alongside) the dashboard:

```bash
websocat ws://127.0.0.1:8765/ws/alerts
```

## Run against a live RTSP camera

```bash
make run-rtsp RTSP="rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"
# equivalently: ./scripts/run_rtsp.sh "rtsp://..."
```

`run_rtsp.sh` builds, ensures the model exists, then **generates a one-camera
config** from your URL and starts the system with the viewer enabled. Useful for
validating a real camera before a full Jetson deployment.

Environment overrides:

| Variable      | Default                         | Meaning |
|---------------|---------------------------------|---------|
| `FSS_DEVICE`  | `cpu`                           | `cuda` to request the GPU EP (adds `-DFSS_ORT_WITH_CUDA=ON`) |
| `FSS_MODEL`   | `models/yolov8n_forklift.onnx`  | path to the ONNX model |

```bash
FSS_DEVICE=cuda make run-rtsp RTSP="rtsp://..."
```

Validate the RTSP URL independently first:

```bash
ffprobe "rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"
```

## Troubleshooting

| Symptom                                   | Fix |
|-------------------------------------------|-----|
| `websocat: Connection refused`            | The app isn't running, or you used the MJPEG port. Alerts are **8765**; the dashboard is **8088**. |
| Dashboard loads but no video              | Confirm the camera has `enable_viewer: true` and `viewer.enabled: true`. |
| `Load model ... failed. File doesn't exist`| Run `scripts/setup_test.sh` (Test) or `scripts/download_model.sh` (RTSP). |
| `RtspCameraSource: failed to open`        | Bad URL/creds — verify with `ffprobe` first. |
| Many `Dropped frame, queue full` warnings | Inference can't keep up; use `yolov8n`, raise `workers`, or lower `target_fps`. |

See also the top-level [README troubleshooting](../../README.md#8-troubleshooting).
