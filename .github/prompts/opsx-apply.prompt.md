---
description: Implement the tasks from the current OpenSpec change's tasks.md
argument-hint: "[change-name]"
allowed-tools: Read, Write, Edit, Bash, Glob, Grep
model: sonnet
---

Implement the tasks defined in the current OpenSpec change. The arguments are: $ARGUMENTS

## Instructions

1. Find the active change (name from `$ARGUMENTS` or most recently modified non-archived dir).

2. Read `meta.yaml` for the `artifacts` map and `schema`. Read all artifacts (tasks required, brief/proposal required, spec/design if full).

3. Read `openspec/config.yaml` for quality commands. Load **every** convention doc whose `applyTo` glob matches a file you will create or edit.

4. Read root `AGENTS.md` for layering rules and gotchas.

5. Implement each unchecked task in `tasks.md`, respecting:
   - **Layering** (`conventions/architecture.md`): interface → application → domain; infrastructure implements ports; no third-party includes in `domain/` or `application/` headers.
   - **C++ style** (`conventions/cpp.md`): `[[nodiscard]]` on `Result<T>` returns; RAII everywhere; no raw `new`/`delete`.
   - **Naming** (`conventions/naming.md`): types `PascalCase`, functions `snake_case`, members `snake_case_`, constants `kCamelCase`.
   - **Error handling** (`conventions/error-handling.md`): `shared::Result<T>` at boundaries; exceptions only for programmer errors; never swallow.
   - **Threading** (`conventions/threading.md`): no `detach`; never hold lock across inference; one engine per worker.
   - **Logging** (`conventions/logging.md`): `LOG_INFO/WARN/ERROR/DEBUG`; include `camera_id`; no PII, no RTSP URLs.
   - **Tests** (`conventions/testing.md`): domain tests 100 % branch; application tests with fakes.
   - **Performance** (`conventions/performance.md`): no allocation/formatting/exceptions on hot path.

6. After implementation, run quality gates:
   ```bash
   make fmt
   clang-tidy -p build $(git diff --name-only HEAD -- '*.cpp' '*.h')
   cmake --build build -j
   ctest --test-dir build --output-on-failure
   ```
   Fix any issues before finishing.

7. Check off completed tasks in `tasks.md`. Update `meta.yaml#status` to `applied`.

8. Summarize what was implemented and suggest `/opsx-review`.
