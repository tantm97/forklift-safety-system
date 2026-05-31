---
description: Start a new OpenSpec change
argument-hint: "<change-name> [--schema lite|full]"
allowed-tools: Read, Write, Edit, Glob, Bash
model: opus
---

Start a new OpenSpec change. The arguments are: $ARGUMENTS

## Instructions

1. Parse the arguments:
   - Extract the change name (kebab-case slug). Reject names with `/`, spaces, or uppercase.
   - Detect `--schema full` or `--schema lite` (default: `lite`).

2. Read `openspec/config.yaml` to load project context, the `conventions.files[]` map, and artifact output paths.

3. Create the change directory: `openspec/changes/<change-name>/`

4. Create `openspec/changes/<change-name>/meta.yaml`:
   ```yaml
   change: <change-name>
   schema: <lite|full>
   status: planning
   created: <today YYYY-MM-DD>
   affected_layers: []   # domain | application | infrastructure | shared | interface
   artifacts:
     brief: openspec/changes/<change-name>/brief.md
   ```

5. Read the appropriate template:
   - Lite: `openspec/templates/lite/brief.md`
   - Full: `openspec/templates/full/proposal.md`

6. Fill the template with **real** content from:
   - The change name itself and any user context
   - `openspec/config.yaml`, root `AGENTS.md`
   - Relevant docs under `docs/development/conventions/` for the layers you expect to touch
   - Real paths under `include/forklift/`, `src/`, `tests/`

   Replace every `{{placeholder}}`. Do NOT leave `<!-- -->` blocks empty.

7. Update `openspec/changes/README.md` — add a row to the Index table.

8. Show the user:
   - Path of the created change directory and artifact
   - Chosen schema
   - Suggested branch name (e.g. `feat/add-cooldown-policy`)
   - Suggest running `/opsx-continue` next
