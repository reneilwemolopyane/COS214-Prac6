# Design decisions and revision history

Format per entry: problem · original design · change · why · diagrams to update.

## R1 Generic vs industry adapters (from Practical 6)
Adapters split into generic technology adapters and industry-specific ones; `ConnectorRegistry` added. *Implemented:* the generic `RecordLookupAdapter` serves policy, police, student records and income checks; only the registered **names** and stub data are industry-specific. Update: class diagram, Adapter diagram, Activity 1.

## R2 Routing decisions were not executable
* **Problem:** the definition could say what depends on what, but "write-off or repair?", "merit or need?" and "fund exhausted?" were prose; `rules` was an unused string map.
* **Original:** dependencies only; amount/branch rules documented, not evaluated.
* **Change:** per-item **guards** written in a tiny rule language (Interpreter: `Expression`, `ExpressionParser`). The `InstanceCoordinator` evaluates a guard when an item's prerequisites are resolved. False → item is **Skipped** (new `SkippedState`, `EVT_SKIPPED`). **Dead-path elimination:** an item whose prerequisites were *all* skipped is skipped too; a join with some completed and some skipped prerequisites runs.
* **Why:** research (WfMC reference model; workflow patterns "exclusive choice / simple merge") shows routing is configuration. New rules never edit the engine.
* **Diagrams:** State diagram (+`Skipped`, `Escalated → Assigned`), class diagram (Interpreter, `ItemConfig`), Activity 1 (guard evaluation, skip), new sequence diagram.

## R3 Approval chain made real
* **Problem:** `ApprovalHandler`/`LevelApprover` existed only in tests; three named classes in the report (`HandlerApprover`, `ManagerApprover`, `SeniorManagerApprover`).
* **Change:** one configurable `LevelApprover` and `ApprovalRouter` building the chain from `"Role=limit;…"`. The coordinator routes an approval item to the responsible level and re-routes on timeout (`escalateWorkItem`).
* **Why:** levels and limits differ per organisation (R20 000/100 000/∞ vs R10 000/50 000/150 000/∞) – same classes, different text. Report class names updated.

## R4 Decorators wired to running items
Definitions list decorators as text (`audit,validation:<key>,security:<Role>`). `WorkflowInstance::applyDecorations` wraps the **instance's cloned** items through `DecoratorFactory` (template stays undecorated, so instances are isolated) and commands act through the outermost component (`frontOf`). `WorkflowComponent::collectItems/underlyingItem/replaceChild` keep this polymorphic (no `dynamic_cast` chains).

## R5 Strict state machine at the engine, friendly at the UI
An action the state forbids is **refused, explained, and audited** (`EVT_REFUSED`), never crashes and never has side effects: the engine validates the transition **before** calling an external system (a refused *complete* cannot charge a payment). The TUI/API do not auto-start items, so the state machine is visible.

## R6 Scenarios separated from the engine
`tividy_scenarios` is a separate library; `Scenario::install` is a Template Method; each industry gets its own `Workspace` (engine + repository + facade) so they cannot interfere; `Session` is shared by the TUI and the web API.

## R7 Memento made generic
The report named `InsuranceClaimInstance`/`ClaimInstanceMemento`. Implemented generically: `WorkflowInstance` (originator), `InstanceMemento`, `ProcessDataStore` (caretaker). Rollback restores states, data **and the assignee** (participant id stored in the item snapshot). External calls are not undone by rollback; **cancel** compensates them (Command `undo`).

## R8 Escalation semantics
Non-approval items escalate to a configured role (`escalationRole`, e.g. missing documents → Claims Handler / Bursary Officer) via `Assigned → Escalated → Assigned`; with no role configured the legacy behaviour (escalate and close) remains. "Nobody higher" is a refusal, not an incident.

## R9 Visitors
`SLAComplianceVisitor` from the Practical 6 report is **not implemented**; `BottleneckVisitor` carries the SLA thresholds. Report updated to match.

## R10 Composite does real work: stage modes order the process
* **Problem:** `Stage` had SEQUENTIAL/PARALLEL modes, but running instances were driven only by explicit dependencies, so the modes did nothing.
* **Change:** `Stage::collectOrdering()` (polymorphic on `WorkflowComponent`) turns a SEQUENTIAL group into ordering between its children; the definition exposes it as `getImpliedDependencies()`; each instance receives it as *orderings*. Orderings only **gate** an item; they never decide a dead path (that is decided by the explicit flow dependencies). Builder-created stages default to PARALLEL, so older definitions behave exactly as before; scenarios opt in (`buildProcessMode`, `buildStageMode`). A nested `SubProcess` ("Repair") is built with `buildSubProcess` / `buildEndSubProcess`. The validating builder checks the combined graph for cycles.
* **Why:** the structure of the definition (not only a dependency list) now controls execution. The intake of both scenarios has no explicit Lodge->Verify->Collect dependencies at all.

## R11 Deadlines are automatic
* **Problem:** escalation after "no response" only happened when a person pressed *escalate*.
* **Change:** `ItemConfig.deadlineSeconds`; the coordinator arms a deadline when an item is assigned and `WorkflowEngine::advanceClock()` (simulated time) escalates every expired item to the next approval level or the configured role, then arms a fresh deadline for the new owner. A `PriorityDecorator` shortens the deadline of the item it wraps (`adjustDeadline`). At the top of the chain nothing loops: the request stays and a ROUTED note is written.
* **UI:** TUI menu 5/9, TUI time-advance actions.

## R12 No manual type or name checks in the engine
Replaced: name comparison chains on states (now `WorkItemState::disposition()`, `isRejection()`, `WorkItemState::byName()` table), `dynamic_cast` when walking stages (now `WorkflowComponent::asStage()`), string-dispatch of item actions in the TUI and API (now one `ItemActions` table), scenario lookup by `if` (now a factory table). Remaining `switch`es are over a closed enum (`Disposition`) or menu numbers.

## R13 Blocked work after a rejection
When an item is rejected, everything that waited for it is closed with the existing *Escalated* state ("needs a person", reason "prerequisite … was closed") and the instance outcome is REJECTED. This was the Practical 6 behaviour (`MediatorStrategyTests` assert it) and is kept deliberately; the audit trail carries the reason.

## R14 Verified quality gates
The whole suite also runs clean under AddressSanitizer + UndefinedBehaviorSanitizer + LeakSanitizer, and the engine, scenarios, TUI and API compile warning-free with `-Wall -Wextra -Wpedantic` in strict C++11. The sanitizer run found and fixed a real leak in `WorkflowBuilder::reset()` (a partly built product was never freed).
