# OpenSpec Setup for forklift-safety-system

This module configures **OpenSpec** for AI-assisted, spec-driven development on
`forklift-safety-system` — a C++17 / OpenCV / ONNX Runtime AI Box for real-time
warehouse forklift safety monitoring.

The structure mirrors the workflow used in `reharp-sync-api` (Go REST), adapted
for a C++ AI pipeline system:

| Reharp (Go REST)              | Forklift (C++ AI)                        |
|-------------------------------|------------------------------------------|
| Handler / Service / Repo      | Interface / Application / Infrastructure |
| GORM migrations               | Model artefact + class-map versioning    |
| Swagger annotations           | WebSocket alert schema doc               |
| `go vet`, `gofmt`             | `clang-tidy`, `clang-format`             |
| `go test`                     | `ctest` + GoogleTest (planned)           |

## Quick Start

```bash
/opsx-new FSS-NNN-add-cooldown-policy
/opsx-continue
/opsx-apply
/opsx-review
/opsx-archive
```

Use `--schema full` for cross-cutting work (new inference backend, new sensor
type, threading model change).

## Workflows

### Lite (default)
`brief → tasks → apply → review → archive`

### Full
`proposal → spec → design → tasks → apply → review → archive`

## Directory Structure

```
openspec/
├── README.md            # This file
├── AGENTS.md            # OpenSpec knowledge base
├── AUTHORING.md         # How to write artifacts
├── config.yaml          # Project context, conventions, quality gates
├── schemas/
│   ├── full.yaml
│   └── lite.yaml
├── models/
│   └── routing.yaml     # Model recommendations per artifact type
├── templates/
│   ├── full/            # proposal / spec / design / tasks
│   └── lite/            # brief / tasks
├── examples/
│   └── example-add-cooldown-policy/
└── changes/
    ├── README.md
    ├── _archived/
    └── <change-name>/
        ├── meta.yaml
        ├── brief.md         (lite)
        ├── proposal.md      (full)
        ├── spec.md          (full)
        └── tasks.md
docs/
├── design/<change-name>.md   # Persistent design docs (full schema)
├── adr/<NNN>-<slug>.md       # Architecture decision records
└── architecture/             # Long-lived architecture references
AGENTS.md                     # Project-wide AI knowledge base (root)
```

## Quality Gates (verified per change)

| Gate            | Command                                                  |
|-----------------|----------------------------------------------------------|
| Format          | `find include src tests -name '*.h' -o -name '*.cpp' \| xargs clang-format --dry-run -Werror` |
| Lint            | `clang-tidy` (from `compile_commands.json`)              |
| Build           | `cmake --build build`                                    |
| Tests           | `ctest --test-dir build --output-on-failure`             |
| ONNX export     | `python scripts/export_yolov8.py …` (if model retrained) |

## Branch Naming

```
<type>/<kebab-case-slug>
```

Types: `feature` · `bugfix` · `hotfix` · `chore` · `refactor` · `perf`.

Examples: `feature/FSS-12-add-cooldown-policy`, `perf/reduce-onnx-allocs`.

## What this OpenSpec adapts from `reharp-sync-api`

- **Same** workflow shape: lite/full + brief/proposal/spec/design/tasks.
- **Same** templates with sections renamed for the C++ / pipeline domain.
- **Same** `config.yaml` schema (`context`, `conventions`, `rules`,
  `quality gates`, `branch pattern`).
- **Adapted** module references: replaced Gin/GORM handler/service/repo
  layering with Clean Architecture (domain / application / infrastructure /
  interface / shared).
- **Adapted** quality gates: replaced `gofmt`/`go vet`/`swag init` with
  `clang-format`/`clang-tidy`/CMake build/CTest.
- **Adapted** "migrations" with "model artefact versioning" — schema
  evolution in YOLO class indices is treated the same way as SQL migrations.
