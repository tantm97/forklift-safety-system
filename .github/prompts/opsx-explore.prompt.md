---
description: Investigate a problem or area of the codebase before starting a change
argument-hint: "<topic or question>"
allowed-tools: Read, Bash, Glob, Grep
model: sonnet
---

Explore and investigate the codebase. The topic is: $ARGUMENTS

## Instructions

1. Read the root `AGENTS.md` (`WHERE TO LOOK` table) to find relevant code.

2. Explore relevant areas:
   - Use Glob/Grep to find related files in `include/forklift/`, `src/`, `tests/`.
   - Read key files to understand current behaviour.
   - Trace data flow: `RtspCameraSource` → `ConcurrentQueue` → inference worker → `RiskDetectionService` → `AlertPublisher`.
   - Look for similar patterns (`src/application/RiskDetectionService.cpp` is the canonical domain-logic example).

3. Produce a structured investigation report:
   - **Problem / Question** — what was investigated
   - **Findings** — what the code currently does
   - **Relevant Files** — `path:line` references
   - **Root Cause** *(for bugs)* — what causes the issue
   - **Layer impact** — which layers will change (domain / application / infrastructure / shared / interface)
   - **Recommendation** — approach + which schema (`lite` / `full`) fits best

4. Suggest `/opsx-new <slug>` (with `--schema full` if multiple layers are affected or a new port is needed).
