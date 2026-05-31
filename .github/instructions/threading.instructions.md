---
applyTo: '**/*.{h,cpp}'
description: 'Threading rules for forklift-safety-system'
---

> **Source of truth:** [`docs/development/conventions/threading.md`](../../docs/development/conventions/threading.md)

Read the full threading document before writing or editing concurrent code.

Key rules (quick reference):
- No `std::thread::detach`. Every thread is owned by an RAII object that joins on destruction.
- No global mutable state. Singletons constructed in `main`, passed by reference.
- Never hold a `std::mutex` lock across inference, RTSP read, or any blocking I/O.
- `std::atomic<bool>` for stop flags only. Anything richer needs a mutex or a queue.
- One `InferenceEngine` instance per worker thread (ORT sessions are not thread-safe).
- `ConcurrentQueue<T>` is the preferred producer-consumer primitive. Default policy: `kDropOldestWhenFull`.
- `ThreadPool::submit` preferred over `std::async` (deterministic shutdown).
- Shutdown order: set stop flag → close queue → join capture thread → shutdown ThreadPool → stop publisher.
- Reviewer checklist: who owns each thread? What does each lock protect? Is the critical section minimal? What happens under producer > consumer rate?
