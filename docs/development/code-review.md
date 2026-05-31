# Code Review

## Reviewer checklist

### Architecture
- [ ] Layer dependency rule respected? (interface → application → domain;
      infrastructure implements ports; no upward includes.)
- [ ] New port in `application/` instead of a concrete dependency?
- [ ] No business logic in `interface/main.cpp`?

### Correctness
- [ ] Inputs validated at boundaries (config, RTSP, WS messages)?
- [ ] All `Result<T>` returns checked at the call site?
- [ ] No swallowed errors?

### Concurrency (see [threading.md](conventions/threading.md))
- [ ] Who owns each new thread? Where is it joined?
- [ ] What does each new lock protect? Is the critical section minimal?
- [ ] No locks held across blocking I/O or inference?
- [ ] Producer/consumer overflow policy explicit?

### Performance
- [ ] Hot path free of allocation / formatting / exceptions?
- [ ] Move used where applicable?
- [ ] New tuning knobs exposed in `conf/system.yaml` if behaviour-affecting?

### Tests
- [ ] Domain changes have unit tests?
- [ ] Application changes have fake-driven tests?
- [ ] Failure paths tested, not just happy path?

### Docs
- [ ] ADR added if the decision is architectural?
- [ ] `docs/architecture/*` updated if pipeline / API / threading changed?
- [ ] Alert schema bumped if payload changed?
- [ ] OpenSpec `tasks.md` ticked off?

### Style
- [ ] `make fmt` clean?
- [ ] Names follow [naming.md](conventions/naming.md)?
- [ ] Header guards correct? Forward declarations used where possible?

## Author etiquette

- Keep PRs ≤ 400 lines of diff where humanly possible.
- One change per PR — refactors and feature work go in separate PRs.
- Self-review the diff before requesting reviewers.
- Respond to every comment; resolve only after the reviewer agrees.
