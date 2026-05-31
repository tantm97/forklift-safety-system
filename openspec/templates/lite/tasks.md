# Tasks: {{change-name}}

## Implementation

- [ ]

## Tests

- [ ] Add/extend tests under `tests/<layer>/` covering the change
- [ ] `ctest --test-dir build --output-on-failure` passes

## Quality Gates

- [ ] `make fmt` is clean
- [ ] `clang-tidy` reports no new warnings
- [ ] `cmake --build build` succeeds with default options
- [ ] Pipeline / WS schema docs updated if shapes changed

## Completion

- [ ] Self-review the diff
- [ ] Open pull request
- [ ] Verify each acceptance criterion in `brief.md`
