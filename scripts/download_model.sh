#!/usr/bin/env bash
# Convenience helper to pull the default base model. The forklift-specific
# weights must be trained by the user — see docs/architecture/pipeline.md.
set -euo pipefail

MODELS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/models"
mkdir -p "$MODELS_DIR"

# Resolve a Python interpreter that has ultralytics installed.
PYTHON=""
for candidate in python3 /opt/anaconda3/bin/python3 /usr/local/bin/python3; do
    if command -v "$candidate" &>/dev/null && "$candidate" -c "import ultralytics" 2>/dev/null; then
        PYTHON="$candidate"
        break
    fi
done

if [[ -z "$PYTHON" ]]; then
    echo "ERROR: no Python with 'ultralytics' found. Run: pip install ultralytics onnx" >&2
    exit 1
fi
echo "Using Python: $PYTHON ($(${PYTHON} --version))"

if [[ ! -f "$MODELS_DIR/yolov8n.pt" ]]; then
    echo "Downloading yolov8n.pt …"
    curl -L --fail \
        "https://github.com/ultralytics/assets/releases/download/v8.2.0/yolov8n.pt" \
        -o "$MODELS_DIR/yolov8n.pt"
fi

echo "Exporting to ONNX …"
"$PYTHON" "$(dirname "${BASH_SOURCE[0]}")/export_yolov8.py" \
    --weights "$MODELS_DIR/yolov8n.pt" \
    --output  "$MODELS_DIR/yolov8n_forklift.onnx"

echo "done — $MODELS_DIR/yolov8n_forklift.onnx"
