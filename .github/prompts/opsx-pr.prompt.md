---
description: Generate a PR description from OpenSpec change artifacts
argument-hint: "[change-name] [--create] [--base <branch>]"
allowed-tools: Read, Bash, Glob
model: sonnet
---

Generate a pull request description from the current OpenSpec change artifacts. The arguments are: $ARGUMENTS

## Instructions

1. Find the change:
   - Name from `$ARGUMENTS` (not a flag), or detect from `git branch --show-current` (strip `feat/`, `fix/`, `chore/`, `refactor/`, `perf/` prefix).
   - Also check `openspec/changes/_archived/` if not found in active changes.

2. Read `meta.yaml` for the `artifacts` map.

3. Read available artifacts:
   - `brief`/`proposal` → Summary + What changed
   - `tasks`            → Implementation checklist
   - `design` (if exists) → Technical approach (one-line summary)

4. Generate a PR description:
   ```markdown
   ## Summary

   <1–3 sentences from the brief/proposal Summary>

   ## What Changed

   - <bullet per major change, with file paths>

   ## Technical Approach
   <!-- only when design.md exists -->

   <one-paragraph summary>

   ## Test Plan

   - [ ] `make fmt`
   - [ ] `cmake --build build -j`
   - [ ] `ctest --test-dir build --output-on-failure`
   - [ ] Acceptance criteria from brief/spec manually verified
   - [ ] Alert schema_version bumped if payload changed

   ## OpenSpec

   - Change: `openspec/changes/<change-name>/`
   - Design: `docs/design/<change-name>.md` *(full schema only)*
   ```

5. If `--create` is in `$ARGUMENTS`:
   - Determine base branch (`--base <branch>` flag, default `main`).
   - Create the PR with `gh pr create`:
     ```bash
     gh pr create --title "<type>(<scope>): <subject>" --base <branch> --body "$(cat <<'EOF'
     <generated description>
     EOF
     )"
     ```
   - Return the PR URL.

6. Otherwise, print the PR description for the user to copy.
