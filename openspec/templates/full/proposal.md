# Proposal: {{change-name}}

## Summary

<!-- One-line description. -->

## Why

<!-- Problem / motivation. Link incidents, profiling reports, customer pain. -->

## What Changes

### Affected Layers

- [ ] domain                (include/forklift/domain/*, src/domain/*)
- [ ] application           (include/forklift/application/*, src/application/*)
- [ ] infrastructure/ai     (ONNX, TensorRT, post-processing)
- [ ] infrastructure/video  (RTSP, decoding, capture)
- [ ] infrastructure/transport (WebSocket, alert serialisation)
- [ ] infrastructure/config (YAML loader, schema)
- [ ] infrastructure/logging
- [ ] interface             (src/interface/main.cpp, composition root)
- [ ] shared                (ThreadPool, ConcurrentQueue, Result)
- [ ] build system          (CMakeLists.txt, options)
- [ ] model artefacts       (models/, class-map)
- [ ] Other:

### Capabilities

1.
2.
3.

## Impact

### Breaking Changes

<!-- WebSocket schema additions/removals, config keys, public header changes. Or "None". -->

### Dependencies

<!-- New CMake packages, new system libraries, minimum compiler version. -->

### Security Considerations

<!-- RTSP credential handling, WebSocket auth, model file integrity. -->

### Performance Impact

<!-- Expected delta on FPS-per-camera, memory, CPU/GPU utilisation. -->

## Out of Scope

## Open Questions
