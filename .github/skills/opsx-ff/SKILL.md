---
name: opsx-ff
description: "Fast-forward: generate all planning artifacts at once"
---

---
description: "Fast-forward: generate all planning artifacts at once"
argument-hint: "<change-name> [--schema lite|full]"
allowed-tools: Read, Write, Edit, Glob, Bash
model: opus
---

Fast-forward an OpenSpec change by generating ALL planning artifacts in one go. The arguments are: $ARGUMENTS

## Instructions

1. Parse arguments — extract change name and `--schema` (default: `lite`).
   If the change directory doesn't exist, create it and `meta.yaml` first (same shape as `/opsx-new`).

2. Read `openspec/config.yaml` and `openspec/changes/<change-name>/meta.yaml`.

3. Skip artifacts that already exist.

4. Generate ALL missing planning artifacts in sequence from `openspec/templates/<schema>/`:
   - **Lite:** `brief` → `tasks`
   - **Full:** `proposal` → `spec` → `design` → `tasks`

   Each artifact must be fully filled in with concrete, project-specific content. Each builds on the previous.

5. Save each artifact to its correct output path.

6. Update `meta.yaml`:
   - Set `status: ready-to-apply`
   - Populate the full `artifacts` map.

7. Update `openspec/changes/README.md`. If `design` was created, update `docs/design/README.md`.

8. Show a summary of all artifacts created and suggest `/opsx-apply`.
