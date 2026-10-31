# Functional requirements → design → pattern → test

| ID | Requirement | Classes | Pattern(s) | Tests |
|---|---|---|---|---|
| FR1 | Definitions separate from independent instances | `WorkflowDefinition`, `WorkflowInstance`, `DefinitionRepository` | Prototype, Builder | `Instances from one definition are isolated`, `WorkflowBuilderTests` |
| FR2 | Stages, work items, nesting, parallel work | `Stage`, `SubProcess`, `WorkItem` | Composite | `StateCompositeTests`, both full-path integration tests |
| FR3 | Work-item lifecycle; invalid actions refused | `WorkItem`, `*State` | State, Singleton | `Invalid actions are refused…`, `A refused completion has no side effects`, `StateCompositeTests` |
| FR4 | Participants, roles, assignment | `Participant`, `WorkAssigner`, strategies | Strategy | `MediatorStrategyTests`, approval tests |
| FR5 | Dependencies, parallel joins | `InstanceCoordinator` | Mediator | `a join releases only after every prerequisite completes` |
| FR6 | Decisions / routing at run time | `Expression*`, `ExpressionParser`, `InstanceCoordinator` | Interpreter, Mediator | `repair path…`, `write-off path (dead-path elimination)`, `Education: merit and need`, `exhausted fund waitlists`, `InterpreterTests` |
| FR7 | Approval by amount, escalation on no response | `ApprovalRouter`, `LevelApprover`, `escalateWorkItem` | Chain of Responsibility, Command | `Approval chain picks the level by amount…`, `Education … Dean`, `ApprovalRouter` tests |
| FR8 | Rejection/failure/cancellation with recovery | `RejectWorkItemCommand`, `CommandProcessor`, `IncidentMonitor`, `cancel` | Command, Observer | `Rejection path…`, `Payment: outage… cancel reverses`, `Education: unregistered student` |
| FR9 | Extra behaviour (validation, security, audit) | `DecoratorFactory`, decorators | Decorator | `Validation decorator…`, `Security decorator…`, `ChainDecoratorTests` |
| FR10 | External systems through one boundary, retries | `ExternalConnector`, adapters, `ConnectorRegistry`, `InvokeServiceCommand` | Adapter, Command | `AdapterTests`, payment outage test |
| FR11 | Events, notifications, history | `WorkflowEvent`, `AuditLogger`, `NotificationService` | Observer | `ObserverEngineTests`, TUI smoke test |
| FR12 | Progress and bottleneck reports | `AuditReportVisitor`, `BottleneckVisitor` | Visitor | `VisitorMementoTests` |
| FR13 | Undo / checkpoints | `InstanceMemento`, `ProcessDataStore` | Memento | `VisitorMementoTests` |
| FR14 | Two industries on one engine; industry rules separate | `scenarios/*`, `Workspace`, `Session` | Template Method, Builder | `Both industries use the same engine classes…`, `TuiSmoke_*` |
| FR15 | Interactive UI over the engine | `tui/main.cpp` | Facade | `TuiSmoke_Insurance_Completes`, `TuiSmoke_Education_Rejects` |
| FR16 | Escalation when nobody responds (automatic) | `InstanceCoordinator::checkDeadlines`, `WorkflowEngine::advanceClock`, `ItemConfig.deadlineSeconds` | Mediator, Chain of Responsibility, Decorator (Priority) | `Deadlines: time passing escalates…`, `Deadlines + Priority decorator…`, `Deadlines: an item picked up in time…`, `TuiSmoke_Deadline_Escalates` |
| FR17 | Structure of the definition controls ordering | `Stage::collectOrdering`, `SubProcess`, `WorkflowDefinition::getImpliedDependencies` | Composite | `Composite: a SEQUENTIAL stage orders…`, `Composite: … nested SubProcess…` |
| FR18 | Safe under memory/UB checking | – | – | run `ctest` in a build with `-fsanitize=address,undefined` (see README) |
