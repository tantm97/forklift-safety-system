# Model artifacts

This directory holds ONNX (and optionally TensorRT engine) artefacts used at
runtime. Files in this directory are gitignored — only this README is tracked.

## Expected files

| File                          | Purpose                              |
|-------------------------------|--------------------------------------|
| `yolov8n_forklift.onnx`       | Default object detector (CPU/GPU)    |
| `yolov8n_forklift.engine`     | Optional TensorRT engine (NVIDIA)    |
| `class_names.txt`             | One class name per line, in model index order |

## Class index contract

The application maps model class indices to `forklift::domain::ObjectClass`
in `OnnxInferenceEngine::initialize`. If you retrain with a different class
ordering, update both the model and the `class_map` initialisation (see
`src/infrastructure/ai/OnnxInferenceEngine.cpp`) and add an ADR
(`/opsx-adr` → `docs/adr/`).

## Generating an ONNX model

```bash
# From the project root
pip install ultralytics
python scripts/export_yolov8.py \
    --weights yolov8n.pt \
    --output  models/yolov8n_forklift.onnx \
    --imgsz   640
```

See `docs/adr/0002-inference-backends.md` for the rationale on supporting
ONNX Runtime and TensorRT side-by-side.
