# What to change in the Practical 6 PDF to produce the final report (`report/`)

The final PDF must match the code. Do these edits, then export to `report/TIVIDY_final_report.pdf`.

## 1. Corrections so the report matches the implementation
| Report says | Change to |
|---|---|
| "supplied as configuration in `main.cpp` or data files" | "supplied as configuration text in `scenarios/*Director.cpp` (guards, approval levels, decorators, service bindings) and scenario classes (people, stub adapters)" |
| `HandlerApprover`, `ManagerApprover`, `SeniorManagerApprover` | one `LevelApprover`; chain built by `ApprovalRouter` from `"Role=limit;…"` |
| Adapters `PolicyAdminAdapter`, `PoliceVerificationAdapter`, `ValuationAdapter`, `PanelBeaterPortalAdapter` (moved to a scenario folder) | generic `RecordLookupAdapter` registered as `policy-admin`, `police-system`, `student-records`, `income-verification`; `PaymentAdapter`, `MessagingAdapter`. (Valuation and panel-beater portal are not simulated: the write-off decision is the *Review claim and decide* outcome.) |
| Memento: `InsuranceClaimInstance`, `ClaimInstanceMemento` | `WorkflowInstance` (originator), `InstanceMemento`, `ProcessDataStore` (caretaker) |
| Visitor: `SLAComplianceVisitor` | remove (`AuditReportVisitor`, `BottleneckVisitor` only) |
| State diagram | add `Skipped` (final) with `skip [guard false / dead path]` from `Created`; add `Escalated → Assigned` (assign); keep `Suspended → Cancelled` (exists in code) |
| Composite description | add: SEQUENTIAL/PARALLEL stages now order execution (`collectOrdering`); nested `SubProcess` "Repair"; no explicit intake dependencies |
| Mediator description | colleagues are `WorkItem` and `WorkAssigner` only; the coordinator also owns guards, approval routers and deadlines. Remove `ApprovalHandler` and `ExternalConnector` as colleagues |
| Strategy description | `DataRuleStrategy` exists but neither scenario uses it |
| Decorator description | `PriorityDecorator` shortens deadlines; `NotificationDecorator` and `AuditDecorator` write to the instance decorator log; `DeadlineDecorator` exists but the engine clock is used |
| Patterns list | add Interpreter, Singleton, Template Method (see `docs/PATTERNS.md`, 17 patterns); legend must include them |
| Activity 3 / sequence 2 | the code has 16 insurance items (added *Fraud investigation*, *Approve settlement amount*, *Authorise repair*, *Authorise write-off payout*) |
| Sequence diagram 2 (*Completing a work item*) | remove the empty `alt` fragment; use `docs/diagrams/sequence_guard_routing.mmd` |
| References | Temporal 2026, django-workflow-kit n.d., ServiceNow n.d., EmpowerID n.d. are cited in the text but missing from the list; Zur Muehlen 2004, Schiefer 2003, Koksal 1998, Rolfe 2001 are listed but not cited – cite or remove |

## 2. New sections required by the final specification
* **Second industry (Education)**: process, participants, branching, failure paths, outcome – the text already exists in Task 2; add `docs/diagrams/education_activity.mmd` (redraw in Visual Paradigm), the approval/escalation sequence (`sequence_approval_escalation.mmd`) and the updated state diagram.
* **Updated requirements** and **traceability**: `docs/REQUIREMENTS_TRACEABILITY.md`.
* **Implementation explanation**: engine vs scenarios split, `Session`/`Workspace`, how a guard, an approval and a decorator stack are configured (`docs/DESIGN_DECISIONS.md` R2–R9).
* **Reuse statement**: "Both industries reuse every class outside `scenarios/` without modification" (`docs/PATTERNS.md`).
* **Design decision history**: R1–R9.
* **Build/run/test instructions**: from `README.md`; plus Git evidence (commit counts per member, ≥3 closed PRs, branching process = `CONTRIBUTING.md`) and the CI/Docker bonus.
* **Contribution statement**: extend with who built the engine changes, TUI, web API/UI, tests, and the education scenario (each member needs ≥10 commits and tests).

## 3. Diagram exports
The `.mmd` files here are sources, **not** the Visual Paradigm deliverables: redraw/refresh the activity, class, sequence and state diagrams in VP, save the updated `.vpp`, export PNG/PDF at a readable size (the class diagram must be exported per subsystem or at large size) and put them in `docs/diagrams/`.
