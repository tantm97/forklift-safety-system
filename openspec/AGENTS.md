# OpenSpec AGENTS — forklift-safety-system

This document is loaded by AI assistants (Copilot CLI, Claude Code) before
authoring or applying an OpenSpec change in this repository.

## Where things live

- Project knowledge base:      `../AGENTS.md` (at repo root)
- Layer conventions:           `../docs/development/conventions/`
- Long-lived architecture:     `../docs/architecture/`
- Decision records:            `../docs/adr/`
- Per-change design docs:      `../docs/design/`
- Workflow definitions:        `schemas/lite.yaml`, `schemas/full.yaml`
- Artifact templates:          `templates/lite/`, `templates/full/`
- Worked example:              `examples/example-add-cooldown-policy/`

## Layer rules (Clean Architecture, strict)

```
interface ────►  application ────►  domain
                       ▲
                       │
              infrastructure
```

- **domain/** never includes anything outside `forklift::domain`. No OpenCV,
  no ORT, no I/O, no logging — pure value types and rules.
- **application/** depends only on `domain` and `shared`. Ports
  (`InferenceEngine`, `VideoSource`, `AlertPublisher`) live here.
- **infrastructure/** implements ports. It is the ONLY layer allowed to
  `#include <onnxruntime_cxx_api.h>`, `<opencv2/...>`, networking headers.
- **interface/** wires everything in the composition root (`main.cpp`).
- **shared/** is leaf-level utilities — must not depend on any other layer.

If a change crosses these boundaries, escalate to the `--schema full`
workflow and document the decision in `docs/adr/`.

## Threading rules (read before any pipeline change)

1. One capture thread per camera. Owns the `cv::VideoCapture` exclusively.
2. Inference workers belong to a per-camera `ThreadPool`. They are the only
   threads allowed to call `InferenceEngine::infer`.
3. Cross-thread handoffs use `forklift::shared::ConcurrentQueue<T>`.
4. `OverflowPolicy::kDropOldestWhenFull` is the default — a stale frame is
   always worse than no frame.
5. Domain types are value types — copy across thread boundaries; never share
   a non-const reference.

## Quality gates (must pass before review)

```bash
make fmt
clang-tidy -p build $(git diff --name-only main... | grep -E '\.(h|cpp)$')
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## When to write an ADR

Always, when:
- Adding or removing a value in `forklift::domain::ObjectClass`
- Adding or removing an `application::InferenceEngine` backend
- Changing the threading model of a pipeline stage
- Changing the WebSocket alert JSON schema (bump version field)

## Common pitfalls

- Forgetting to update `models/README.md` after a class-map change.
- Calling `cv::Mat` methods inside the domain layer (use `Frame` only at the
  application boundary).
- Holding a queue lock across an inference call — never do this; pop first,
  release the lock, then infer.
