# Tasks: {{change-name}}

## Pre-Implementation

- [ ] Review and approve `proposal.md`
- [ ] Review and approve `spec.md`
- [ ] Review and approve `docs/design/{{change-name}}.md`
- [ ] Create feature branch matching the configured pattern

## Implementation Tasks

### Domain (`include/forklift/domain/`, `src/domain/`)

- [ ] Add/modify value types
- [ ] Update tests in `tests/domain/`

### Application (`include/forklift/application/`, `src/application/`)

- [ ] Add/modify use case or port
- [ ] Maintain Clean Architecture: NO infrastructure includes
- [ ] Add tests in `tests/application/`

### Infrastructure (`src/infrastructure/<group>/`)

- [ ] Add/modify adapter
- [ ] Confirm header includes are guarded behind feature flags if optional
- [ ] Add tests in `tests/infrastructure/<group>/` (or a fake if the dep is heavy)

### Interface (`src/interface/main.cpp`)

- [ ] Wire the new component into the composition root
- [ ] Surface new flags / config keys

### Build (`CMakeLists.txt`)

- [ ] Add sources to `forklift_core`
- [ ] Add/adjust `find_package`, options, or compile definitions
- [ ] Confirm both default and `-DFSS_WITH_TENSORRT=ON` configures succeed

### Model artefacts (`models/`)

- [ ] Update `models/README.md` if the class-map changed
- [ ] Add an ADR documenting any change to `ObjectClass`

### Documentation

- [ ] Update `docs/architecture/pipeline.md` if pipeline shape changed
- [ ] Update `docs/architecture/websocket-api.md` if alert schema changed
- [ ] Update `README.md` if developer-facing setup changed

## Quality Gates

- [ ] `make fmt` is clean
- [ ] `clang-tidy` reports no new warnings
- [ ] `cmake --build build` succeeds
- [ ] `ctest --test-dir build --output-on-failure` passes

## Verification

- [ ] Each acceptance criterion in `spec.md` is observably met
- [ ] Manual multi-camera smoke against `conf/system.yaml` (or a local fixture)
- [ ] Edge cases listed in `spec.md` exercised

## Completion

- [ ] Self-review diff
- [ ] Open pull request
- [ ] Address review feedback
- [ ] Run archive workflow once merged
