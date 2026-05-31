---
description: Verify implementation matches the change artifacts
argument-hint: "[change-name]"
allowed-tools: Read, Bash, Glob, Grep
model: sonnet
---

Verify that the implementation matches the planned artifacts. The arguments are: $ARGUMENTS

## Instructions

1. Locate the active change. Read all artifacts via `meta.yaml#artifacts`.

2. Check implementation against tasks:
   - Are all tasks in `tasks.md` checked off?
   - Does the code match the design in `docs/design/<change>.md` (if full schema)?
   - Are all acceptance criteria from `brief.md` / `spec.md` observably met?

3. Run quality gates:
   ```bash
   make fmt
   cmake --build build -j
   ctest --test-dir build --output-on-failure
   ```

4. Generate a verification report:
   - Checklist of each requirement / acceptance criterion with ✅ or ❌
   - Any missing items or gaps
   - Recommended next step (`/opsx-archive` if passed, fix list if not)

5. Update `meta.yaml#status` to `reviewed` if all checks pass.
