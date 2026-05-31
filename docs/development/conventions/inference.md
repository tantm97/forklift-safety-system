# Inference

## Model

- **Architecture**: YOLOv8 (Ultralytics)
- **Classes**: `0 = person`, `1 = forklift`
- **Input**: 640×640 RGB, NCHW, float32, normalised to [0, 1]
- **Output**: `[1, 4 + num_classes, num_anchors]` — channels are
  `cx, cy, w, h, cls0, cls1, ..., clsN-1`

Export with `scripts/export_yolov8.py`.

## Postprocessing

`infrastructure::ai::YoloV8Postprocessor`:

1. Transpose output if needed to `[num_anchors, 4 + num_classes]`.
2. For each anchor: `score = max(class scores)`, `cls = argmax(...)`.
3. Discard anchors with `score < confidence_threshold`.
4. Convert `(cx,cy,w,h)` → `(x,y,w,h)` (top-left form).
5. `cv::dnn::NMSBoxes` with `iou_threshold`.
6. Map class index → `domain::ObjectClass` via the class map.

## Class map

Currently hard-coded in `OnnxInferenceEngine::initialize` (see
[ADR-0002 consequences](../../adr/0002-inference-backends.md)). Move to
config in a follow-up change — file the brief as
`change-class-map-from-config`.

## Adding a backend

1. Add a new class under `infrastructure/ai/` that implements
   `application::InferenceEngine`.
2. Reuse `YoloV8Postprocessor` — do not duplicate NMS.
3. Add a CMake option `FSS_WITH_<BACKEND>` and wire selection from
   `inference.backend` in `main.cpp`.
4. Update [ADR-0002](../../adr/0002-inference-backends.md).

## Adding a class

1. Re-train / fine-tune the model so the new class is at a stable index.
2. Update the class map.
3. Add the constant to `domain::ObjectClass`.
4. Decide whether the new class triggers a safety zone or is a person-like
   entity — update `RiskDetectionService` accordingly.
5. Bump the alert schema if the alert payload changes.
