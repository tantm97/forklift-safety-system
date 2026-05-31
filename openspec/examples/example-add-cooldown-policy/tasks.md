# Tasks: example-add-cooldown-policy

## Implementation

- [ ] Add `alert_cooldown` to `PipelineConfig` (already present — confirm default 2000 ms)
- [ ] In `FrameProcessingPipeline::inference_loop`, keep a
      `std::unordered_map<int, Alert::TimePoint> last_alert` per worker thread
      and skip publishing when `(now - last_alert[forklift_track_id]) < cooldown`
- [ ] Plumb `alert_cooldown_ms` through `YamlConfigLoader`

## Tests

- [ ] `tests/application/frame_processing_pipeline_test.cpp`
  - Feed 10 s × 15 FPS of synthetic detections (1 forklift + 1 person inside zone)
  - Assert 5 alerts emitted with `alert_cooldown = 2000 ms`
  - Re-enter after 3 s of no-overlap → assert 1 fresh alert on first overlap
- [ ] `ctest --test-dir build --output-on-failure` passes

## Quality Gates

- [ ] `make fmt` clean
- [ ] `clang-tidy` reports no new warnings
- [ ] `cmake --build build` succeeds with default and `-DFSS_WITH_ONNXRUNTIME=OFF`

## Docs

- [ ] `docs/architecture/pipeline.md` — add "Cooldown semantics" subsection
- [ ] `conf/system.yaml` — document the key inline

## Completion

- [ ] Self-review
- [ ] Open PR titled `feat(application): add per-(camera, forklift) alert cooldown`
- [ ] Run archive workflow once merged
