---
applyTo: 'tests/**/*_test.cpp'
description: 'Testing conventions for forklift-safety-system'
---

> **Source of truth:** [`docs/development/conventions/testing.md`](../../docs/development/conventions/testing.md)

Read the full testing document before writing or editing tests.

Key rules (quick reference):
- Use the zero-dependency harness in `tests/test_harness.h` (`TEST_CASE`, `EXPECT_TRUE`, `EXPECT_EQ`, `EXPECT_NEAR`). No GoogleTest/Catch2 unless a design doc justifies it.
- Domain tests (`tests/domain/`): 100 % branch coverage; no OpenCV, no ORT, no RTSP.
- Application tests (`tests/application/`): use hand-rolled fakes (`FakeInferenceEngine`, `FakeAlertPublisher`). Never hit real model, real camera, or real WebSocket.
- Infrastructure smoke tests: gated behind a build flag (`FSS_BUILD_INFRA_TESTS`); require real ORT.
- `TEST_CASE` names describe behaviour, not function names: `"Alert is suppressed within cooldown window"`.
- Every new `Result<T>` return path needs a test for both the `.ok()` and `.error()` branches.
- No `std::this_thread::sleep_for` in tests — use deterministic clocks or fake sources.
- Run with: `ctest --test-dir build --output-on-failure`.
