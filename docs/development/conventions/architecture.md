# Architecture Conventions

This document codifies the layering rules introduced in
[ADR-0001](../../adr/0001-clean-architecture.md). It is the rulebook the
OpenSpec workflow points reviewers and AI assistants at.

## Layer dependency rule

```
interface  →  application  →  domain
                  │            ▲
                  ▼            │
              shared      infrastructure (implements application ports)
```

A `#include` may only point **down** this graph. Up-pointing includes are
a CI failure.

## Where things live

| You are adding...                          | Layer            | Folder example                                       |
|--------------------------------------------|------------------|------------------------------------------------------|
| A new value type (e.g. `TrackId`)          | domain           | `include/forklift/domain/TrackId.h`                  |
| A new use case (e.g. heat-map aggregator)  | application      | `include/forklift/application/HeatmapService.h`      |
| A new port (e.g. metrics sink)             | application      | `include/forklift/application/MetricsSink.h`         |
| A new adapter (e.g. Prometheus sink)       | infrastructure   | `src/infrastructure/metrics/PrometheusSink.{h,cpp}`  |
| A wiring change                            | interface        | `src/interface/main.cpp`                             |
| A reusable primitive (e.g. ring buffer)    | shared           | `include/forklift/shared/RingBuffer.h`               |

## When in doubt

- If it has business meaning → **domain**.
- If it orchestrates business logic → **application**.
- If it talks to the outside world (network, files, GPU) → **infrastructure**.
- If it's mechanical (data structure, allocation, RAII helper) → **shared**.

## Anti-patterns we reject in review

- A `domain/` header that includes `<opencv2/dnn.hpp>` or `<onnxruntime/*>`.
- An `application/` header that names a concrete adapter class.
- An `infrastructure/` header that includes another adapter's header.
- A `main.cpp` that contains any conditional business logic (`if person
  count > 5`). Move it into `application/`.
