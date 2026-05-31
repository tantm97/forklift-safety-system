---
name: opsx-continue
description: Create the next artifact in the current OpenSpec change workflow
---

---
description: Create the next artifact in the current OpenSpec change workflow
argument-hint: "[change-name]"
allowed-tools: Read, Write, Edit, Glob, Bash
model: opus
---

Continue the current OpenSpec change by creating the next artifact. The arguments are: $ARGUMENTS

## Instructions

1. Find the active change:
   - If `$ARGUMENTS` provides a name, use `openspec/changes/<change-name>/`.
   - Otherwise, find the most recently modified directory under `openspec/changes/` that is NOT `_archived`.

2. Read `meta.yaml` to determine schema and which artifacts already exist.

3. Determine the next artifact:
   - **Lite:** `brief` → `tasks`
   - **Full:** `proposal` → `spec` → `design` → `tasks`

4. Read the template from `openspec/templates/<schema>/`. Read all existing artifacts for context.

5. Generate the next artifact with real, concrete content. Reference actual paths under `include/forklift/`, `src/`, `tests/`, `conf/`. No placeholder blocks left empty.

6. Save to the correct location:
   - `brief`/`proposal` → `openspec/changes/<change-name>/brief.md` or `proposal.md`
   - `spec`             → `openspec/changes/<change-name>/spec.md`
   - `design`           → `docs/design/<change-name>.md`
   - `tasks`            → `openspec/changes/<change-name>/tasks.md`

7. Update `meta.yaml` artifacts map. If `tasks.md` was just created, set `status: ready-to-apply`.

8. Update `openspec/changes/README.md` Status column.

9. Tell the user what was created. If `tasks.md` was created, suggest `/opsx-apply`.
