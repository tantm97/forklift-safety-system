#!/usr/bin/env bash
# setup_test.sh — one-command setup for offline AI testing.
#
# Downloads the ONNX model and four forklift test video clips if not already
# present. The first clip is downloaded via yt-dlp; clips 2–4 are trimmed
# variants of clip 1 using ffmpeg (different start offsets so each "camera"
# shows a slightly different point in the footage).
#
# Safe to re-run — skips steps that are already complete.
#
# Usage:
#   ./scripts/setup_test.sh
#   make setup-test          # equivalent via Makefile

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "=== forklift-safety-system test setup ==="

# ── 1. ONNX model ─────────────────────────────────────────────────────────────
if [ -f "models/yolov8n_forklift.onnx" ]; then
    echo "[1/3] model: already exists (models/yolov8n_forklift.onnx) ✓"
else
    echo "[1/3] model: exporting yolov8n_forklift.onnx..."
    ./scripts/download_model.sh
fi

# ── 2. Base test video (clip 01) ──────────────────────────────────────────────
mkdir -p test_data

if [ -f "test_data/forklift_01.mp4" ]; then
    echo "[2/3] video 01: already exists ✓"
else
    echo "[2/3] video 01: downloading forklift test video..."
    DEST="test_data/forklift_01.mp4" ./scripts/download_test_video.sh
fi

# ── 3. Variant clips 02–04 (trimmed from clip 01) ────────────────────────────
make_variant() {
    local n="$1"
    local offset="$2"
    local dest="test_data/forklift_0${n}.mp4"
    if [ -f "$dest" ]; then
        echo "[3/3] video 0${n}: already exists ✓"
        return
    fi
    echo "[3/3] video 0${n}: creating variant (offset ${offset}s)..."
    if command -v ffmpeg &>/dev/null; then
        ffmpeg -y -loglevel error \
            -ss "$offset" -i test_data/forklift_01.mp4 \
            -t 90 -c copy \
            "$dest" 2>&1 || {
            # copy failed (key-frame alignment) — re-encode
            ffmpeg -y -loglevel error \
                -ss "$offset" -i test_data/forklift_01.mp4 \
                -t 90 -c:v libx264 -preset ultrafast -crf 28 -c:a copy \
                "$dest" 2>&1
        }
        echo "    done → $dest"
    else
        echo "    ffmpeg not found — copying clip 01 as fallback"
        cp test_data/forklift_01.mp4 "$dest"
    fi
}

make_variant 2 10
make_variant 3 20
make_variant 4 30

echo ""
echo "=== Setup complete ==="
echo "Run:  make run-test"
echo "  or: ./build/forklift_safety --config conf/system_test.yaml"
echo "Dashboard: http://127.0.0.1:8088"
echo "Alerts WS: websocat ws://127.0.0.1:8765/ws/alerts"
