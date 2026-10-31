# GoF patterns: participants as implemented (17)

All patterns run inside the same engine; none is an isolated demo. "Runs when" says where a marker sees it.

| # | Category | Pattern | Participants (real classes) | Runs when |
|---|---|---|---|---|
| 1 | Creational | **Builder** | `WorkflowBuilder` ← `ValidatingWorkflowBuilder` (used by scenarios; checks rules, approval text, decorators and cycles incl. stage ordering), `DraftWorkflowBuilder` (lenient alternative, unit-tested); Director `WorkflowDirector` ← `MotorClaimWorkflowDirector`, `EducationBursaryWorkflowDirector`; Product `WorkflowDefinition` | `Scenario::install` |
| 2 | Creational | **Prototype** | `WorkflowComponent::clone()` ← `WorkItem`, `Stage`, `SubProcess`; client `WorkflowDefinition::createInstance` | every instance start (isolation) |
| 3 | Creational | **Singleton** | `WorkItemState::instance()` of each concrete state (`CreatedState` … `SkippedState`) | every transition |
| 4 | Structural | **Composite** | `WorkflowComponent`, leaf `WorkItem`, composites `Stage`, `SubProcess` (nested "Repair" group in insurance). `Stage::collectOrdering` turns a SEQUENTIAL group into ordering between its children (PARALLEL adds none); decorators and tree walks go through `collectItems`/`asStage` instead of casts | start of every instance (intake and stage order), visitors, memento |
| 5 | Structural | **Facade** | `WorkflowFacade` over `DefinitionRepository`, `WorkflowDefinition`, `WorkflowInstance`, `WorkflowEngine` | TUI/API start |
| 6 | Structural | **Adapter** | target `ExternalConnector`; adapters `RecordLookupAdapter`, `PaymentAdapter`, `MessagingAdapter`, …; adaptees `RecordLookupAPI`, `PaymentAPI`, `MessagingAPI`; `ConnectorRegistry` | item service calls, payments, notifications |
| 7 | Structural | **Decorator** | `WorkItemDecorator` ← `ValidationDecorator`, `SecurityDecorator`, `AuditDecorator`, `PriorityDecorator`, `NotificationDecorator` (`DeadlineDecorator` exists and is unit-tested; the engine's own deadline clock is used instead); built from definition text by `DecoratorFactory` | collect documents (validation), release payment (security + audit), approvals (priority shortens the deadline, audit, notification), fraud investigation (priority) |
| 8 | Behavioural | **State** (each state also says what it *means* to the mediator via `disposition()`, so no code compares state names) | `WorkItemState` ← `CreatedState`, `AvailableState`, `AssignedState`, `InProgressState`, `SuspendedState`, `EscalatedState`, `CompletedState`, `RejectedState`, `CancelledState`, `SkippedState`; context `WorkItem` | every action; invalid ones refused |
| 9 | Behavioural | **Observer** | subject `WorkflowSubject`/`WorkflowInstance`; observers `DependencyTracker`, `AuditLogger`, `NotificationService`, `IncidentMonitor` | every event |
| 10 | Behavioural | **Mediator** | `WorkflowMediator` ← `InstanceCoordinator`; colleagues `WorkItem` (reports COMPLETED/REJECTED/SKIPPED… through `notify`) and `WorkAssigner` (picks people for what the coordinator releases). The coordinator also owns the guards, approval routers and deadlines for its instance | completion → release, guards, approval routing, escalation, deadlines |
| 11 | Behavioural | **Command** | `WorkflowCommand` ← `CompleteWorkItemCommand`, `StartWorkItemCommand`, `RejectWorkItemCommand`, `EscalateWorkItemCommand`, `AssignWorkItemCommand`, `InvokeServiceCommand`; invoker `CommandProcessor` | every action; retries; cancel → undo |
| 12 | Behavioural | **Strategy** | `AssignmentStrategy` ← `RoleBasedStrategy` (default), `ShortestQueueStrategy`, `RoundRobinStrategy`, `DataRuleStrategy` (available and unit-tested; neither scenario needs a data-driven rule); context `WorkAssigner` | assignment (`shortest-queue` on *Verify police report*, `round-robin` on *Reference check*) |
| 13 | Behavioural | **Chain of Responsibility** | `ApprovalHandler` ← `LevelApprover`; request `ApprovalRequest`; built by `ApprovalRouter` from `"Role=limit;…"` | *Approve settlement amount* / *Approve bursary award*, escalation |
| 14 | Behavioural | **Interpreter** | `Expression` ← `ComparisonExpression`, `AndExpression`, `OrExpression`, `NotExpression`; context `EvaluationContext`; client `ExpressionParser` | guards (branch / skip decisions) |
| 15 | Behavioural | **Visitor** | `WorkflowVisitor` ← `AuditReportVisitor`, `BottleneckVisitor`, registration/snapshot visitors; elements `WorkflowInstance`, `Stage`, `WorkItem`, `ProcessDataStore` | TUI "progress reports" |
| 16 | Behavioural | **Memento** | originator `WorkflowInstance`; memento `InstanceMemento` (+`StageMemento`, `WorkItemMemento`); caretaker `ProcessDataStore` | "undo last step" |
| 17 | Behavioural | **Template Method** | `Scenario::install()` fixes the order; subclasses `InsuranceScenario`, `EducationScenario` override `director()`, `addParticipants()`, `addConnectors()` | startup |

Category check (spec): creational 3, structural 4, behavioural 10; at least five from the required list (Observer, Mediator, Command, Adapter, Chain of Responsibility, Builder, Interpreter, Facade, Visitor, Singleton).

## Generic engine vs industry configuration
Reused by both industries **without modification**: everything except `scenarios/` – `WorkflowEngine`, `WorkflowInstance`, `InstanceCoordinator`, `WorkItem` and states, `Stage`, `ApprovalRouter`/`LevelApprover`, `ExpressionParser`, `DecoratorFactory`, all adapters, observers, visitors, `ProcessDataStore`, `WorkflowFacade`.
Industry-specific (only here): the two directors (text for guards, approval levels, decorators, service bindings), the people, the stub adaptee data, the sample cases.
