# Architecture Decision Records (ADRs)

Numbered, append-only records of significant architectural decisions in
`forklift-safety-system`. Create new ones via the OpenSpec `/opsx-adr`
workflow.

Each ADR follows the format:

```
# ADR-NNN: <Title>

**Date:**     YYYY-MM-DD
**Status:**   Proposed | Accepted | Deprecated | Superseded by ADR-XXX
**Category:** Architecture | Convention | Pattern | Workaround
**Change:**   <change-name or N/A>

## Context
## Decision
## Consequences
```

## Index

| ADR | Title | Status | Date |
|-----|-------|--------|------|
| [0001](0001-clean-architecture.md) | Adopt Clean Architecture in five layers | Accepted | 2026-05-23 |
| [0002](0002-inference-backends.md) | Support ONNX Runtime and TensorRT side-by-side | Accepted | 2026-05-23 |
| [0003](0003-frame-drop-oldest.md)  | Drop-oldest frame queue policy as default | Accepted | 2026-05-23 |
