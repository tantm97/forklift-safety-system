# Brief: example-add-cooldown-policy

## What

Add a configurable cooldown (default 2000 ms) per (camera_id, forklift_track_id)
inside `FrameProcessingPipeline::inference_loop` to suppress duplicate
`person_near_forklift` alerts emitted on consecutive frames.

## Why

Operations reported (incident OPS-118) that a single 30-second incursion produced
~450 WebSocket alerts at 15 FPS, swamping the dashboard. Without de-duplication
the alert stream is unusable by humans and triggers downstream rate-limits on
the notification gateway.

## Scope

- `include/forklift/application/FrameProcessingPipeline.h`
- `src/application/FrameProcessingPipeline.cpp`
- `include/forklift/infrastructure/config/YamlConfigLoader.h`
- `src/infrastructure/config/YamlConfigLoader.cpp`
- `conf/system.yaml`
- `tests/application/frame_processing_pipeline_test.cpp` (new)
- `docs/architecture/pipeline.md` (cooldown section)

## Layer impact

- [x] application
- [x] infrastructure (config)
- [ ] domain
- [ ] interface
- [ ] shared

## Acceptance Criteria

- [ ] When a person stays inside a forklift's safety zone for 10 s at 15 FPS,
      exactly 5 alerts are emitted with a 2000 ms cooldown.
- [ ] The cooldown is configurable per-camera via
      `cameras[].alert_cooldown_ms` in `conf/system.yaml`.
- [ ] When the person leaves the zone and re-enters after > cooldown, a new
      alert is emitted on the first overlapping frame.
- [ ] Pipeline unit test exercises both suppression and re-trigger paths.
- [ ] `docs/architecture/pipeline.md` describes the cooldown semantics.
