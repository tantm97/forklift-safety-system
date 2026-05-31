# Error Handling

Two-tier model.

## Tier 1 — `shared::Result<T>` at module boundaries

Any function that can fail for a *runtime* reason (network down, model not
loaded, decode failure) returns `forklift::shared::Result<T>`:

```cpp
[[nodiscard]] Result<Frame> RtspCameraSource::read() {
    cv::Mat mat;
    if (!cap_.read(mat) || mat.empty()) {
        return Result<Frame>::error("rtsp read failed");
    }
    return Result<Frame>::ok(Frame{camera_id_, std::move(mat), now()});
}
```

Callers MUST check `.ok()` (compile-warning enforced by `[[nodiscard]]`).
`Result<void>` is specialised for fallible operations with no return value.

## Tier 2 — exceptions for programmer errors

Throw `std::logic_error`, `std::invalid_argument`, or
`std::runtime_error` for situations that indicate a bug:

- Indexing past the end of a fixed-size array
- Passing a null `InferenceEngine` to a pipeline that requires one
- Failing to parse a config file at startup (fail fast, abort)

Exceptions do not cross thread boundaries — wrap worker bodies in
`try { ... } catch (const std::exception& e) { LOG_ERROR(...); }`.

## What never to do

- Do not return error codes via output parameters (`bool foo(Result*)`).
- Do not log-and-swallow. Always log AND return an error.
- Do not `throw` for control flow.
- Do not catch `(...)` outside of thread-loop top-frames and `main`.

## Error context

`Result::error_message()` should be human-readable and prefixed with the
component name:

```
"OnnxInferenceEngine: failed to load model 'models/yolov8n.onnx'"
"RtspCameraSource[cam-dock-01]: timeout after 5000 ms"
```
