# Threading topology

## Threads per process

For `N` cameras and `W` inference workers per camera:

| Thread group           | Count       | Purpose                                  |
|------------------------|-------------|------------------------------------------|
| Capture                | N           | One per camera; drives `cv::VideoCapture`|
| Inference worker       | N × W       | Pops from per-camera frame queue, runs engine |
| WebSocket I/O          | 1 (library) | Owned by the WS library (uses its own pool internally) |
| Main                   | 1           | Composition, signal handling             |

Default for 16 cameras with W=1 ⇒ 16 capture + 16 inference + 1 WS + 1 main = **34 threads**.

## Ownership rules

1. A `cv::VideoCapture` is owned **exclusively** by its capture thread.
   No other thread may call methods on it.
2. An `InferenceEngine` instance is **not thread-safe**. Use one instance
   per worker, OR check the implementation's `backend_name()` docs for an
   explicit thread-safety contract.
3. `RiskDetectionService` and `SafetyZoneService` are immutable after
   construction — safe to share across all cameras.
4. `AlertPublisher` MUST be thread-safe (the WebSocket implementation
   internally serialises sends).

## Synchronisation primitives

- `ConcurrentQueue<T>`: `std::mutex` + two `std::condition_variable`s.
  Never hold the lock across a heavy operation — pop first, release, infer.
- `ThreadPool`: single FIFO task queue with `std::mutex` + cv. Workers join
  on destruction.
- `std::atomic<bool>` flags (`running_`, `stop_`) drive shutdown.

## Shutdown sequence

1. SIGINT/SIGTERM sets the global stop flag.
2. `main` calls `Pipeline::stop()` for each camera, in any order.
3. Each pipeline:
   1. Sets `running_ = false`.
   2. `frames_.close()` — wakes any blocked consumer.
   3. Joins the capture thread.
   4. `inference_pool_.shutdown()` — drains and joins workers.
   5. `source_->close()`.
4. `main` calls `publisher->stop()`.

All threads exit deterministically; no detached threads, no leaked
resources.

## TSAN / sanitizers

Build with `-fsanitize=thread` for CI smoke runs:

```bash
cmake -S . -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -O1 -g"
cmake --build build-tsan -j
ctest --test-dir build-tsan
```
