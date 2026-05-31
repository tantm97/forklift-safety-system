#!/usr/bin/env bash
# setup_test.sh — one-command setup for offline AI testing.
#
# Downloads the ONNX model and forklift test video if not already present.
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
    echo "[1/2] model: already exists (models/yolov8n_forklift.onnx) ✓"
else
    echo "[1/2] model: exporting yolov8n_forklift.onnx..."
    ./scripts/download_model.sh
fi

# ── 2. Test video ─────────────────────────────────────────────────────────────
if [ -f "test_data/forklift_test.mp4" ]; then
    echo "[2/2] video: already exists (test_data/forklift_test.mp4) ✓"
else
    echo "[2/2] video: downloading forklift test video..."
    ./scripts/download_test_video.sh
fi

echo ""
echo "=== Setup complete ==="
echo "Run:  make run-test"
echo "  or: ./build/forklift_safety --config conf/system_test.yaml"
echo "Then: websocat ws://127.0.0.1:8765/ws/alerts"
