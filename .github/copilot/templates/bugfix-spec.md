# Bugfix Spec: [Title]

## Summary

<!-- One-line description of the bug and fix -->

## Bug Report

- **Symptom**: 
- **Reproduction steps**: 
- **Expected**: 
- **Actual**: 
- **First seen**: 
- **Camera / environment**: 

## Root Cause

<!-- Which layer? Which function? What invariant was violated? -->

## Affected Files

- 

## Fix Approach

<!-- Minimal, surgical change. Reference the relevant convention doc. -->

## Regression Risk

<!-- What else could this break? What existing tests cover the area? -->

## Acceptance Criteria

- [ ] Symptom is no longer reproducible
- [ ] Existing tests still pass

## Quality Gates

- [ ] `make fmt`
- [ ] `cmake --build build -j`
- [ ] `ctest --test-dir build --output-on-failure`
