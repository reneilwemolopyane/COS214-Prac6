# TIVIDY – generic workflow management system (COS 214, 2026)

One reusable **C++11** workflow engine running **two industries**:

| Industry | Workflow | Definition id |
|---|---|---|
| Insurance | Motor vehicle accident claim | `motor-claim` |
| Education | University bursary application | `bursary-application` |

Everything industry-specific lives in `scenarios/` (configuration text, participants, stub adapters).
The engine (every other module) contains no industry names. Details: [`docs/PATTERNS.md`](docs/PATTERNS.md),
[`docs/DESIGN_DECISIONS.md`](docs/DESIGN_DECISIONS.md), [`docs/REQUIREMENTS_TRACEABILITY.md`](docs/REQUIREMENTS_TRACEABILITY.md).

## Dependencies
* A C++11 compiler (GCC ≥ 7, Clang ≥ 6, MSVC 2019), **CMake ≥ 3.10**, Make/Ninja
* Tests use **doctest** (vendored in `third_party/doctest`; nothing to install)
* Optional: Doxygen (API docs), Docker

## Build
```bash
cmake -S . -B build
cmake --build build -j
```
Targets: `tividy_tui` (text UI), `tividy_engine`, `tividy_scenarios` and the test executables.

## Run – interactive text UI (both industries)
```bash
./build/tividy_tui            # add --no-color for plain output
```
`1` start a workflow → pick a sample case → `1` do work on an item (start / complete / reject / escalate / act as someone else / call its external system).
Switch industry with `3`; switch a stub external system off with `4` to see retries and an incident.
Inside an instance: history, notifications/incidents/commands, progress reports (Visitors), undo last step (Memento), cancel with payment reversal (Command undo).

A scripted run (both industries, an exception path in each) is `./build/tividy_tui --no-color < tests/tui_smoke.txt`.

### Suggested demo
| Industry | Case | What it shows |
|---|---|---|
| Insurance | `repair-small` | happy path; write-off branch **skipped** |
| Insurance | `writeoff-large` | WRITE_OFF decision skips the repair chain (dead-path elimination); approval climbs to Senior Manager |
| Insurance | `suspicious` | fraud score routes to the investigator |
| Insurance | `lapsed-policy` | start item → *call external system* (valid=false) → reject → rejection cascade |
| Insurance | `missing-docs` | validation decorator refuses; **let 8 days pass** → documents escalate to the Claims Handler automatically |
| Insurance | `repair-small` + let 2 days pass at *Approve settlement amount* | priority halves the deadline; approval climbs a level per expired deadline |
| Education | `merit-funded` / `need-large` | merit vs need route; amount climbs Faculty Officer → … → Dean |
| Education | `fund-exhausted` | award branch skipped, student waitlisted and notified |
| Education | `deregistered` | rejection path |
| both | outage toggle `4` → *payment down* | retries, incident; recover; **cancel** reverses the payment |

## Test
```bash
cd build && ctest --output-on-failure
```
| Suite | Covers |
|---|---|
| `ScenarioIntegrationTests` | **both industries end to end**: routing, dead paths, approval chain, escalation, decorators, adapters, outages, compensation, invalid actions, instance isolation |
| `InterpreterTests` | rule language, approval router |
| `WorkflowBuilderTests` | Builder / Director / Facade / repository |
| `StateCompositeTests`, `ChainDecoratorTests`, `AdapterTests`, `MediatorStrategyTests`, `VisitorMementoTests`, `ObserverEngineTests` | per-pattern and engine wiring |
| `TuiSmoke_*` | the real TUI driven by a recorded keyboard session |

## Quality gates
```bash
# memory / undefined-behaviour checking (all suites clean)
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g -O1"
cmake --build build-asan -j && (cd build-asan && ctest --output-on-failure)
# warnings
cmake -S . -B build-warn -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
```
Time and deadlines are simulated: TUI menu *5 / 9 let time pass*.

## Documentation (Doxygen)
```bash
doxygen Doxyfile && open docs/api/html/index.html
```

## Repository layout
| Folder | Role |
|---|---|
| `core/ mediator/ command/ state/ strategy/ observer/ composite_prototype/ visitor/ adapter/ builder/ facade/ decorator/ chain_of_responsibility/ memento/ interpreter/` | **generic engine** (`tividy_engine`) |
| `scenarios/` | **industry configuration** (`tividy_scenarios`): directors, participants, stub adapters, `Session` |
| `tui/` | interactive text interface |
| `tests/`, `Facade_BuilderTests/` | doctest suites |
| `docs/` | research, UML (`.vpp` + exports), design decisions, traceability |
| `report/` | final PDF report |

## Engineering extras
GitHub Actions CI (`.github/workflows/ci.yml`: C++ build + `ctest` on Linux), a `Dockerfile`, Doxygen config.
See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the branching and review process.
