# Module boundaries

The five layers and their allowed includes.

| Layer          | May include from                              | May NOT include            |
|----------------|-----------------------------------------------|----------------------------|
| domain         | std, `<chrono>`, `<cstdint>`, `<string>`, OpenCV `cv::Mat` ONLY inside `Frame.h` | onnxruntime, networking, infrastructure/ |
| shared         | std                                           | any forklift layer         |
| application    | domain, shared, std                           | infrastructure, OpenCV in headers (Frame.h re-exports the only cv type allowed) |
| infrastructure | application (ports), domain, shared, std, OpenCV, ONNX Runtime, TensorRT, WS libs, yaml-cpp | another infrastructure adapter's private header |
| interface      | everything above                              |                            |

## Compile-time hint

Headers are organised under `include/forklift/<layer>/`. A grep audit:

```bash
# Domain headers must NOT include anything from infrastructure/
! grep -r "include.*infrastructure" include/forklift/domain/
# Application headers must NOT include anything from infrastructure/
! grep -r "include.*infrastructure" include/forklift/application/
```

Add these to CI once `clang-tidy` is wired up — both pass on `main` today.

## Why `Frame.h` may include `cv::Mat`

`cv::Mat` is the de-facto image-buffer abstraction we use everywhere; making
the domain `Frame` own one avoids costly copies across the pipeline.

If we ever need to ship the domain layer without OpenCV (e.g. for a Wasm
analyser of historical alerts), we will introduce a `PixelBuffer` PIMPL.
That decision is deferred — see the "Out of scope" note in
[ADR-0001](../adr/0001-clean-architecture.md).
