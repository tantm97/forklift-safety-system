# Implementing a new feature

End-to-end flow for any change, from idea to merged PR.

## 1. Decide the workflow

| You are doing...                                     | Workflow |
|------------------------------------------------------|----------|
| < 1 day; one layer; no API or schema change          | **lite** |
| Multi-layer, schema change, new dep, perf-critical   | **full** |

The router in `openspec/models/routing.yaml` lists the model that should
generate each artefact when AI-assisted.

## 2. Create the change folder

```
openspec/changes/<change-name>/
├── meta.yaml
├── brief.md        # lite
└── tasks.md
# OR
├── proposal.md     # full
├── spec.md
├── design.md
└── tasks.md
```

See `openspec/examples/example-add-cooldown-policy/` for a worked lite
example, and `openspec/templates/` for blank templates.

## 3. Write the brief / proposal

- Name the **business reason** ("operations report …", "incident OPS-118").
- List **acceptance criteria** in measurable terms.
- Mark **layer impact** — drives which conventions apply.

## 4. Implement

Walk the layers **inside-out** so tests stay green:

1. **domain** — add value types; pure unit tests.
2. **application** — add use cases and ports; tests with fakes.
3. **infrastructure** — wire the real adapter.
4. **interface** — compose in `main.cpp`.

Run after each step:

```bash
make test
```

## 5. Quality gates

Before opening the PR:

```bash
make fmt
clang-tidy -p build $(git diff --name-only main -- '*.cpp' '*.h')
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 6. Update docs

- Architecture-affecting? → add an ADR under `docs/adr/`.
- Alert schema change? → bump `schema_version` and update
  `docs/architecture/websocket-api.md`.
- New convention? → file under `docs/development/conventions/`.

## 7. PR

- Title: `<type>(<scope>): <subject>` — e.g.
  `feat(application): add per-(camera, forklift) alert cooldown`
- Include the OpenSpec change folder in the PR.
- Link the OPS / incident ticket if any.

## 8. Archive

After merge, move the change folder:

```
openspec/changes/<change-name>/
  → openspec/changes/_archived/<YYYY-MM-DD>-<change-name>/
```
