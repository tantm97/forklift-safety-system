# Feature Spec: [Title]

## Summary

<!-- One-line description -->

## Context

<!-- Why is this needed? What problem does it solve? -->

## Requirements

### Functional

1. 
2. 

### Non-Functional

- Performance: ≥ 15 FPS per camera; p95 latency ≤ 100 ms
- Threading: capture thread + inference ThreadPool; no blocking on hot path
- Observability: use `LOG_INFO/WARN/ERROR/DEBUG`; include `camera_id`

## Design

### Affected Layers

- [ ] `domain/`              — new value type or rule
- [ ] `application/`         — new use case or port
- [ ] `infrastructure/ai/`   — inference adapter change
- [ ] `infrastructure/video/` — RTSP source change
- [ ] `infrastructure/transport/` — alert transport change
- [ ] `infrastructure/config/`  — config loader change
- [ ] `shared/`              — primitive (queue, pool, Result)
- [ ] `interface/`           — composition root wiring
- [ ] `conf/system.yaml`     — new config key
- [ ] `tests/`               — new test files

### Approach

<!-- Technical approach and key decisions.
     Reference layered architecture: interface → application → domain;
     infrastructure implements application ports. -->

### API Changes

<!-- New/modified WebSocket alert fields? Bump schema_version if yes.
     Update docs/architecture/websocket-api.md. -->

### Config Changes

<!-- New keys in conf/system.yaml? Document the key, type, unit, and default. -->

## Acceptance Criteria

- [ ] 
- [ ] 

## Testing Plan

- Domain unit tests (pure, no deps):
- Application tests with fakes (`FakeInferenceEngine`, `FakeAlertPublisher`):
- Manual smoke (real RTSP or file source):

## Quality Gates

- [ ] `make fmt` — clang-format clean
- [ ] `clang-tidy` — no new warnings
- [ ] `cmake --build build -j` — succeeds
- [ ] `ctest --test-dir build --output-on-failure` — passes

## Open Questions

- 
