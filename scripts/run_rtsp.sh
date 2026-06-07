#!/usr/bin/env bash
# run_rtsp.sh — laptop/production run against a live RTSP camera.
#
# Generates a one-camera config from your RTSP URL and starts the detector with
# the MJPEG dashboard enabled. Useful for validating a real camera before a full
# Jetson deployment.
#
# Usage:
#   ./scripts/run_rtsp.sh "rtsp://user:pass@10.0.0.11:554/Streaming/Channels/101"
#   FSS_DEVICE=cuda ./scripts/run_rtsp.sh "rtsp://..."   # request CUDA EP
#   FSS_MODEL=models/yolov8n_forklift.onnx ./scripts/run_rtsp.sh "rtsp://..."

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

RTSP_URL="${1:-}"
if [[ -z "$RTSP_URL" ]]; then
    echo "usage: $0 <rtsp-url>" >&2
    exit 2
fi

MODEL="${FSS_MODEL:-models/yolov8n_forklift.onnx}"
DEVICE="${FSS_DEVICE:-cpu}"

CMAKE_ARGS=(-S . -B build -DCMAKE_BUILD_TYPE=Release)
[[ "$DEVICE" == "cuda" ]] && CMAKE_ARGS+=(-DFSS_ORT_WITH_CUDA=ON)

echo "==> Building"
cmake "${CMAKE_ARGS[@]}" >/dev/null
cmake --build build -j

if [[ ! -f "$MODEL" ]]; then
    echo "==> Model $MODEL missing — downloading"
    ./scripts/download_model.sh
fi

CFG="$(mktemp -t fss_rtsp_XXXX).yaml"
trap 'rm -f "$CFG"' EXIT
cat > "$CFG" <<YAML
backend: onnxruntime
log_level: info
inference:
  model_path: $MODEL
  device: $DEVICE
  input_width: 640
  input_height: 640
  conf_threshold: 0.35
  nms_threshold: 0.45
safety_zone:
  mode: relative
  pad_ratio: 0.5
websocket:
  host: 0.0.0.0
  port: 8765
  path: /ws/alerts
viewer:
  enabled: true
  host: 0.0.0.0
  port: 8088
  web_root: web
  target_fps: 15
cameras:
  - id: rtsp-01
    source_type: rtsp
    rtsp_url: "$RTSP_URL"
    queue_capacity: 4
    workers: 1
    alert_cooldown_ms: 2000
    enable_viewer: true
YAML

echo ""
echo "==> Starting (RTSP). Dashboard: http://127.0.0.1:8088/  Alerts: ws://127.0.0.1:8765/ws/alerts"
echo "    Ctrl-C to stop."
echo ""
exec ./build/forklift_safety --config "$CFG"
