# C++ Coding Conventions

Language: **C++17**. We prefer the standard library before any third-party
utility.

## Header hygiene

- Header guard: `FORKLIFT_<LAYER>_<NAME>_H_` (no `#pragma once`).
- Include order: own header, C system, C++ standard, third-party, project.
- Forward-declare in headers when possible; full include in `.cpp`.
- Public headers live under `include/forklift/<layer>/`; private helpers
  live next to their `.cpp` in `src/`.

## Types & values

- Prefer `enum class` over `enum`.
- Prefer `constexpr` over `#define` for constants.
- Use `[[nodiscard]]` for functions returning `Result<T>` or owning handles.
- No raw `new` / `delete`. `std::unique_ptr` for ownership, raw pointers
  for non-owning references, `std::shared_ptr` only when ownership is
  genuinely shared.
- Pass by `const T&` for non-trivial inputs; by value for primitives and
  small POD; by `T&&` only when the function will move.

## Functions

- One responsibility per function. Keep ≤ 60 lines.
- Prefer free functions in an anonymous namespace over private static
  members.
- `noexcept` on move ctors / move assignment.

## RAII everywhere

Every resource (file, socket, thread, GPU buffer) must be owned by an RAII
type. `~T()` is the only place we release.

## Lambdas

- Capture explicitly (`[this, &foo]`). Default-capture `[=]` and `[&]` are
  banned outside very short, local lambdas (≤ 3 lines).

## See also

- [naming.md](naming.md)
- [error-handling.md](error-handling.md)
- [threading.md](threading.md)
- [performance.md](performance.md)
