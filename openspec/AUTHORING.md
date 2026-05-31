# Authoring artifacts (forklift-safety-system)

Practical voice/style guide for OpenSpec artefacts in this repo. The full
workflow philosophy lives in the reference doc (`OPENSPEC_GUIDE.md` from
`reharp-sync-api`); this file is the C++/AI-pipeline-specific adaptation.

## Voice

- Present tense, active voice. "The pipeline drops the oldest frame." not
  "Old frames will be dropped."
- Reference concrete files / classes — `application::FrameProcessingPipeline`
  — not vague nouns like "the pipeline component".
- Use units everywhere: ms, FPS, MB, px.

## Load order (what to read first)

1. `AGENTS.md` (repo root) — always
2. `openspec/AGENTS.md` — for OpenSpec-shaped work
3. `docs/architecture/pipeline.md` — for any pipeline-touching change
4. `docs/development/conventions/<applicable file>` — per the `applyTo` rules
   in `openspec/config.yaml`

## Per-artifact checklist

### brief.md / proposal.md

- **Why** must reference an incident, a profiling number, or an explicit
  customer requirement. "Refactor for cleanliness" is not a reason.
- **Scope** lists real paths (`include/forklift/application/...`).
- For full schema: tick at least one `Affected Layer`.

### spec.md

- One spec per capability. Don't bundle "add cooldown" with "expose metrics".
- Include the C++ signature for new public APIs.
- Include the YAML config snippet for new keys.
- Always include latency / FPS / memory budgets.

### design.md

- Always sketch the data flow even if it's the same as the existing one —
  it forces you to confirm you haven't accidentally crossed a layer.
- The "Threading Model" section is mandatory for any change under `shared/`,
  `application/`, or `infrastructure/video/`.

### tasks.md

- Leaf-actionable: each checkbox should be executable in one sitting.
- Include the quality-gate commands verbatim — don't reference them by name.
- Add a task to update `docs/architecture/` whenever pipeline shape or alert
  schema changes (the reviewer will check this).

## Anti-patterns

- **Hidden coupling.** Don't sneak `#include <opencv2/...>` into a header
  under `include/forklift/domain/` — the boundary is enforced by convention,
  not by the build.
- **Speculative interfaces.** Don't add a virtual hook "in case we need it".
  YAGNI applies harder in real-time systems where each indirection costs ns.
- **Magic numbers in code.** Thresholds (conf, NMS, padding, cooldown) MUST
  be exposed via `conf/system.yaml`.

## Good example

See `examples/example-add-cooldown-policy/` for a fully-filled lite-workflow
change covering a small, contained C++ feature: per-(camera, forklift)
cooldown for alert de-duplication.
