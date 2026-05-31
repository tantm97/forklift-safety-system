---
applyTo: '**/*.{h,cpp}'
description: 'C++17 coding conventions for forklift-safety-system'
---

> **Source of truth:** [`docs/development/conventions/cpp.md`](../../docs/development/conventions/cpp.md)

Read the full C++ conventions document before generating or editing files.

Key rules (quick reference):
- Language: C++17. Standard library first; third-party only when necessary.
- Header guard: `FORKLIFT_<LAYER>_<NAME>_H_` (no `#pragma once`).
- Include order: own header, C system, C++ standard, third-party, project.
- Types `PascalCase`, functions/methods `snake_case`, members `snake_case_`, constants `kCamelCase`, enum values `kCamelCase`, macros `UPPER_SNAKE`.
- Files: `PascalCase.h` / `PascalCase.cpp`; tests: `snake_case_test.cpp`.
- No raw `new`/`delete`. `std::unique_ptr` for ownership, `std::shared_ptr` only when ownership is genuinely shared.
- `[[nodiscard]]` on every function returning `shared::Result<T>` or an owning handle.
- Pass non-trivial inputs by `const T&`; move with `T&&` only when the callee takes ownership.
- No default-capture lambdas (`[=]`, `[&]`) beyond 3-line local lambdas.
- Quality gates: `make fmt` (clang-format), `clang-tidy -p build`, `cmake --build build -j`, `ctest`.
