# Threading Rules

See also [`docs/architecture/threading.md`](../../architecture/threading.md).

## Hard rules

1. **No `std::thread::detach`.** Every thread is owned by an RAII object
   that joins on destruction.
2. **No global mutable state.** Singletons are constructed in `main` and
   passed by reference.
3. **No raw `std::mutex` in domain or application headers.** Synchronisation
   belongs to `shared::ConcurrentQueue` / `shared::ThreadPool` /
   infrastructure adapters.
4. **Never hold a lock across an inference call** or any blocking I/O.
5. **`std::atomic` for flags only.** Anything richer needs a mutex or a
   queue.
6. **Per-thread random / per-thread inference engine.** Do not share these
   across workers.

## Soft rules

- Prefer a `ConcurrentQueue` over hand-rolled `mutex + cv` even for short
  hand-offs.
- Prefer `ThreadPool::submit` over `std::async` (deterministic shutdown).
- Sleep with `std::this_thread::sleep_for`, never busy-wait.

## Reviewer checklist

When reviewing concurrent code, ask:

- [ ] Who owns each thread? Where is it joined?
- [ ] What invariant does each lock protect? Is the critical section
      minimal?
- [ ] Is shutdown deterministic and bounded in time?
- [ ] What happens if a producer is faster than the consumer? (Drop?
      Block? Back-pressure?)
- [ ] Have you run the change under `-fsanitize=thread`?
