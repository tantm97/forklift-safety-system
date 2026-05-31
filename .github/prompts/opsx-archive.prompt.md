---
description: Archive a completed OpenSpec change
argument-hint: "[change-name]"
allowed-tools: Read, Write, Edit, Bash, Glob
model: haiku
---

Archive a completed OpenSpec change. The arguments are: $ARGUMENTS

## Instructions

1. Find the change (from `$ARGUMENTS` or most recently modified non-archived dir).

2. Read `meta.yaml` to confirm it exists and capture the `artifacts` map.

3. Update `meta.yaml`: set `status: archived` and `archived: <today YYYY-MM-DD>`.

4. Move workflow-state files to `openspec/changes/_archived/<change-name>/`:
   ```bash
   mkdir -p openspec/changes/_archived/<change-name>
   mv openspec/changes/<change-name>/* openspec/changes/_archived/<change-name>/
   rmdir openspec/changes/<change-name> 2>/dev/null
   ```

5. Persistent `docs/` files (design docs, ADRs) stay in place — do NOT move them.

6. Update `openspec/changes/README.md`: remove the row. If no active changes remain, restore:
   ```
   | _(none yet)_ | | | |
   ```

7. Confirm the archive succeeded and list the archived path and any persistent docs that remain.
