---
name: opsx-learnings
description: Document a learning as an ADR + AGENTS.md entry
---

---
description: Document a learning as an ADR + AGENTS.md entry
argument-hint: "[learning text — leave empty to scan the current change]"
allowed-tools: Read, Write, Edit, Glob, Bash
model: sonnet
---

Document a learning. Arguments: $ARGUMENTS

## Instructions

1. Determine the learning:
   - If `$ARGUMENTS` is non-empty, treat it as the learning text.
   - Otherwise, find the active change and scan all artifacts for insights worth recording (patterns used, gotchas hit, workarounds applied).

2. Pick the next ADR number from `docs/adr/` (4-digit, zero-padded).

3. Categorise: `Convention` | `Gotcha` | `Pattern` | `Workaround`.

4. Create `docs/adr/<NNNN>-<kebab-title>.md` following the standard ADR format. Include concrete `// BAD` / `// GOOD` C++ snippets if the learning is code-level.

5. Update `docs/adr/README.md` Index.

6. Append to `AGENTS.md` under `## LEARNINGS`:
   ```markdown
   ### [<Category>] <Short title>
   **Context**: <one line>
   **Learning**: <one line>
   **ADR**: docs/adr/<NNNN>-<kebab-title>.md
   ```

7. Confirm what was created and print the ADR path.
