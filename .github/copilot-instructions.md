# GitHub Copilot Instructions — forklift-safety-system

Use [`AGENTS.md`](../AGENTS.md) in the repository root as the single source of truth for project
guidance, structure, and gotchas.

## File-Type Conventions

Always follow the per-file conventions in [`docs/development/conventions/`](../docs/development/conventions/)
when generating or editing code. The `.github/instructions/` stubs auto-apply via Copilot's
`applyTo` mechanism and reference the full convention docs.

| Scope | `applyTo` glob | Convention File |
|-------|----------------|-----------------|
| All C++ files | `**/*.{h,cpp}` | [`conventions/cpp.md`](../docs/development/conventions/cpp.md) |
| All C++ files (architecture) | `**/*.{h,cpp}` | [`conventions/architecture.md`](../docs/development/conventions/architecture.md) |
| All C++ files (naming) | `**/*.{h,cpp}` | [`conventions/naming.md`](../docs/development/conventions/naming.md) |
| All C++ files (error handling) | `**/*.{h,cpp}` | [`conventions/error-handling.md`](../docs/development/conventions/error-handling.md) |
| All C++ files (security) | `**/*.{h,cpp}` | [`conventions/security.md`](../docs/development/conventions/security.md) |
| Test files | `tests/**/*_test.cpp` | [`conventions/testing.md`](../docs/development/conventions/testing.md) |
| Inference adapters | `**/infrastructure/ai/**/*.{h,cpp}` | [`conventions/inference.md`](../docs/development/conventions/inference.md) |
| Transport adapters | `**/infrastructure/transport/**/*.{h,cpp}` | [`conventions/transport.md`](../docs/development/conventions/transport.md) |
| Config adapters | `**/infrastructure/config/**/*.{h,cpp}` | [`conventions/configuration.md`](../docs/development/conventions/configuration.md) |
| YAML config files | `conf/**/*.yaml` | [`conventions/configuration.md`](../docs/development/conventions/configuration.md) |

Conventions that apply to **all** C++ code regardless of glob:

- [`conventions/threading.md`](../docs/development/conventions/threading.md) — thread ownership & synchronisation rules
- [`conventions/logging.md`](../docs/development/conventions/logging.md) — `LOG_INFO/WARN/ERROR/DEBUG` macros
- [`conventions/performance.md`](../docs/development/conventions/performance.md) — hot-path rules, ≥ 15 FPS target

When multiple conventions apply, **follow the stricter rule**.

## Key References

- **All documentation**: `docs/` (single source of truth)
- **Spec-driven workflow**: [`openspec/README.md`](../openspec/README.md), [`openspec/AUTHORING.md`](../openspec/AUTHORING.md)
- **Slash commands**: [`.github/copilot/index.md`](./copilot/index.md)

## Workflow

1. Read `AGENTS.md` first — it lists structure, conventions and gotchas in one place.
2. For any new change, run `/opsx-new <slug>` (lite) or `/opsx-ff <slug> --schema full` (full).
3. Follow the conventions in `docs/development/conventions/` while implementing.
4. Run quality gates before committing:
   - `make fmt` — `clang-format -i` over all sources
   - `clang-tidy -p build <changed files>`
   - `cmake --build build -j`
   - `ctest --test-dir build --output-on-failure`
5. Use `/opsx-review` and `/opsx-verify` before opening a PR; produce the PR description with `/opsx-pr --create`.
