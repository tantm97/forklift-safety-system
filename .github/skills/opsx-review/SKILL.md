---
name: opsx-review
description: Review the implementation against the OpenSpec change specs and tasks
---

---
description: Review the implementation against the OpenSpec change specs and tasks
argument-hint: "[change-name]"
allowed-tools: Read, Bash, Glob, Grep
model: opus
---

Review the implementation of the current OpenSpec change. The arguments are: $ARGUMENTS

## Instructions

1. Find the active change. Read all artifacts via `meta.yaml#artifacts`.

2. Read root `AGENTS.md` and **every convention doc** whose `applyTo` glob matches a changed file. Load `docs/development/code-review.md` as the reviewer checklist.

3. Review against:
   - All acceptance criteria in `brief.md` / `spec.md`
   - All tasks in `tasks.md` — are they all checked off?
   - **Architecture** (`conventions/architecture.md`): no third-party in domain/application headers; no business logic in `interface/`; no upward includes.
   - **C++ style** (`conventions/cpp.md`): RAII; `[[nodiscard]]`; no raw `new`; includes ordered.
   - **Naming** (`conventions/naming.md`): types/functions/members/constants correct.
   - **Error handling** (`conventions/error-handling.md`): all `Result<T>` returns checked; no swallowed errors; context strings prefixed with component name.
   - **Threading** (`conventions/threading.md`): no detached threads; lock critical sections minimal; no lock across inference; shutdown deterministic.
   - **Logging** (`conventions/logging.md`): no hot-path `LOG_INFO`; `camera_id` included; no RTSP URLs or PII.
   - **Tests** (`conventions/testing.md`): domain 100 % branch; application fakes; failure paths tested.
   - **Performance** (`conventions/performance.md`): hot path free of allocation, formatting, exceptions.
   - **Inference** (`conventions/inference.md`, if applicable): one engine per worker; PIMPL; NMS in postprocessor.
   - **Transport** (`conventions/transport.md`, if applicable): `publish()` thread-safe; non-blocking; schema_version bumped if payload changed.
   - **Config** (`conventions/configuration.md`, if applicable): fail-fast validation; no raw yaml-cpp types in public header.
   - **Security** (`instructions/security.instructions.md`): no credentials in logs or JSON; model path validated.

4. Run quality gates:
   ```bash
   make fmt
   cmake --build build -j
   ctest --test-dir build --output-on-failure
   ```

5. Produce structured review:
   - ✅ What looks good
   - ⚠️ Issues found (with `path:line` and citing the convention doc)
   - 🛠️ Suggested fixes

6. Update `meta.yaml#status` to `reviewed` if all checks pass. Suggest `/opsx-archive` when ready.
