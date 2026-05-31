---
applyTo: '**/infrastructure/config/**/*.{h,cpp}'
description: 'Configuration loader conventions for forklift-safety-system'
---

> **Source of truth:** [`docs/development/conventions/configuration.md`](../../docs/development/conventions/configuration.md)

Read the full configuration document before adding or modifying the config loader.

Key rules (quick reference):
- Single config file: `conf/system.yaml`. Loaded once at startup via `YamlConfigLoader`. Read-only after init.
- All YAML keys: `snake_case`. Durations end in `_ms` / `_us`; pixel distances end in `_px`; ratios are unit-less.
- Defaults live in `YamlConfigLoader` — a minimal config (just RTSP URLs) must still produce a working system.
- Validation on load: empty `cameras` list → fail fast; duplicate `cameras[].id` → fail fast; `backend: tensorrt` without `FSS_WITH_TENSORRT` → fail fast with a clear message; missing model file → fail fast.
- Never expose raw `yaml-cpp` types (e.g. `YAML::Node`) in the public header. Return plain structs.
- `FSS_HAS_YAML_CPP` macro is set via CMake `target_compile_definitions ... PUBLIC`. Do NOT `#define` it manually in source.
- Sensitive values (RTSP credentials): document that `chmod 600 conf/system.yaml` is required. Do not add encryption unless an ADR justifies it.
- No hot-reload. Config changes require a process restart. Document this in any user-facing change.
