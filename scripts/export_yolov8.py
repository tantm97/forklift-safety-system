#!/usr/bin/env python3
"""Export an Ultralytics YOLOv8 checkpoint to ONNX.

Usage:
    python scripts/export_yolov8.py \
        --weights yolov8n.pt \
        --output  models/yolov8n_forklift.onnx \
        --imgsz   640

Requires:
    pip install ultralytics onnx
"""
from __future__ import annotations

import argparse
import shutil
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--weights", required=True, help="Path to .pt checkpoint")
    parser.add_argument("--output",  required=True, help="Destination .onnx path")
    parser.add_argument("--imgsz",   type=int, default=640)
    parser.add_argument("--opset",   type=int, default=12)
    args = parser.parse_args()

    from ultralytics import YOLO  # imported lazily so --help works without the dep

    model = YOLO(args.weights)
    onnx_path = Path(model.export(format="onnx", imgsz=args.imgsz, opset=args.opset, dynamic=False))

    dst = Path(args.output)
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.move(str(onnx_path), str(dst))
    print(f"wrote {dst}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
