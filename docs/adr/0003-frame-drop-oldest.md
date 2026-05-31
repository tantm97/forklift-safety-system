# ADR-0003: Drop-oldest frame queue policy as default

**Date:**     2026-05-23
**Status:**   Accepted
**Category:** Pattern
**Change:**   bootstrap

## Context

When inference cannot keep up with capture (e.g. a slow CPU or a sudden
multi-camera burst), the system has three options:

1. Block the capture thread until a slot frees up.
2. Drop the newest frame.
3. Drop the oldest frame.

Real-time safety monitoring cares about NOW, not 30 seconds ago. A stale
alert is worse than a dropped one — operators lose trust in latency.

## Decision

- `forklift::shared::ConcurrentQueue<T>::OverflowPolicy::kDropOldestWhenFull`
  is the default for all inter-stage queues in the pipeline.
- The number of dropped frames is exposed via `ConcurrentQueue::dropped()`
  for observability (will be wired to a metrics sink in a future change).
- Per-camera `queue_capacity` defaults to 4, configurable in
  `conf/system.yaml`.

`kBlockWhenFull` remains available for back-pressure-friendly stages
(e.g. testing harnesses, batch processing).

## Consequences

- ✅ Capture thread is never blocked by a slow inference worker.
- ✅ End-to-end latency stays bounded by `queue_capacity / fps` even under
  load.
- ⚠️ Silent frame loss when the engine is undersized for the target FPS —
  must be visible via the `dropped()` counter and (future) metrics.
- ⚠️ Tests must use deterministic clocks when asserting drop behaviour.
