---
description: Document an Architecture Decision Record
argument-hint: "<decision topic>"
allowed-tools: Read, Write, Edit, Bash, Glob
model: sonnet
---

Create an ADR to document an architectural decision. The topic is: $ARGUMENTS

## Instructions

1. List existing ADRs in `docs/adr/` (pattern `NNN-*.md`) and pick the next number (zero-padded to 4 digits, starting at `0001`).

2. Create `docs/adr/<NNNN>-<kebab-title>.md`:
   ```markdown
   # ADR-<NNNN>: <Title>

   **Date:**     <YYYY-MM-DD>
   **Status:**   Accepted
   **Category:** Architecture | Convention | Pattern | Workaround
   **Change:**   <change-name or N/A>

   ## Context

   <Why did this decision come up? Reference actual incident, constraint, or measurement.>

   ## Decision

   <What was decided. Reference real paths under include/forklift/ and src/.>

   ## Consequences

   <✅ benefits, ⚠️ trade-offs, follow-up actions.>
   ```

3. Update `docs/adr/README.md` Index table with a new row. Replace `_(none yet)_` if present.

4. If the ADR captures a gotcha or pattern worth remembering, append to root `AGENTS.md#LEARNINGS`:
   ```markdown
   ### [<Category>] <Short title>
   **Context**: <one line>
   **Learning**: <one line>
   **ADR**: docs/adr/<NNNN>-<kebab-title>.md
   ```

5. Show the ADR path and suggest next steps.
