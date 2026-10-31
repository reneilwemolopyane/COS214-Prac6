# Contributing / branching and review process

* `main` is always buildable and green (`ctest`). Nobody pushes to `main` directly.
* One branch per task: `feature/<area>-<short-name>`, `fix/<what>`, `docs/<what>`, `test/<what>`.
* Small, meaningful commits (imperative subject, e.g. `Add ApprovalRouter for chain configuration`).
* Open a **pull request** for every branch. Another member reviews (at least one approval); CI must pass. Squash-merge or merge, then delete the branch.
* Every member adds code **and** tests (doctest) for what they build; new patterns need a test and an entry in `docs/PATTERNS.md`.
* Design changes go in `docs/DESIGN_DECISIONS.md` and update the affected diagrams in the same PR.
* Third-party code must be cited (README and report).
