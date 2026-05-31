# Performance

## Targets

- **≥ 15 FPS per camera**, sustained, for 8–16 cameras on the target box.
- **End-to-end p95 latency ≤ 100 ms** (capture → alert publish).
- **No frame backlog**: dropped-frame counter stays bounded under load.

## Design principles

1. **Bounded queues, drop-oldest** — see
   [ADR-0003](../../adr/0003-frame-drop-oldest.md).
2. **One inference engine per worker** — no shared mutable state on the
   hot path.
3. **Batch = 1** by default for predictable latency. Batching is a future
   opt-in change.
4. **`std::move` is free; copies are expensive** — `cv::Mat` is shallow-
   copied but always prefer move when handing a `Frame` between threads.

## What to measure

| Metric                    | How                                |
|---------------------------|------------------------------------|
| FPS per camera            | rolling counter in pipeline        |
| Queue depth / drop count  | `ConcurrentQueue::size/dropped()`  |
| Inference latency p50/p95 | histogram in `OnnxInferenceEngine` |
| WebSocket fan-out latency | publisher histogram                |

Wire to a metrics sink in a dedicated change — do not bake into hot paths
via locking.

## Hot-path don'ts

- No allocation inside the per-frame loop except `cv::Mat` blobs.
- No `std::string` formatting above `DEBUG`.
- No `std::regex`. Ever.
- No exceptions for normal flow.
- No `std::shared_mutex` — we have not measured a case that needed it.

## Tuning knobs

All in `conf/system.yaml`:

```yaml
cameras:
  - id: dock-01
    queue_capacity: 4        # higher = more buffering, more latency
    inference_workers: 1     # raise if your engine is multi-thread-safe
    alert_cooldown_ms: 2000
```

## Profiling

```bash
# CPU
sudo perf record -F 999 -g -- ./build/forklift_safety --config conf/system.yaml
sudo perf report

# Memory
valgrind --tool=massif ./build/forklift_safety --config conf/system.yaml
```
