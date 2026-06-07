#!/usr/bin/env bash
# run_test.sh — laptop test run using a local VIDEO FILE (no camera needed).
#
# Builds the project, downloads the model + test video if missing, then starts
# the detector with conf/system_test.yaml. Open the dashboard to watch the
# annotated stream and alerts.
#
# Usage:
#   ./scripts/run_test.sh            # CPU (default)
#   FSS_CUDA=1 ./scripts/run_test.sh # build with CUDA EP (needs CUDA-enabled ORT)

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

CMAKE_ARGS=(-S . -B build -DCMAKE_BUILD_TYPE=Release)
[[ "${FSS_CUDA:-0}" == "1" ]] && CMAKE_ARGS+=(-DFSS_ORT_WITH_CUDA=ON)

echo "==> Building"
cmake "${CMAKE_ARGS[@]}" >/dev/null
cmake --build build -j

echo "==> Ensuring model + test video"
./scripts/setup_test.sh

echo ""
echo "==> Starting (video file). Open the dashboard:"
echo "      Dashboard : http://127.0.0.1:8088/"
echo "      Alerts WS : ws://127.0.0.1:8765/ws/alerts"
echo "    Ctrl-C to stop."
echo ""
exec ./build/forklift_safety --config conf/system_test.yaml
