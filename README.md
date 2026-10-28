# COS214-Prac6

## Team Members

| Student Name and Surname | Student Number |
|---|---|
| Lizalise Mbonisweni | u23587874 |
| Boikemelo Masoka | u25128648 |
| Thembelisha Skosana | u25224663 |
| Lethabo Molobi | u25090209 |
| Boitumelo Monareng | u25208943 |
| Reneilwe Molopyane | u25161874 |
| Dingalethu Ngumbela | u25170547 |

## Task 1: Research

### 1. Workflow Basics: Definition vs Instance

A Workflow Management System is a system that defines, manages, and executes workflows using software based on the workflow logic. It automates the execution of applications and is robust against performance variations and failures.A workflow definition is a structured blueprint or electronic template that outlines the exact sequence of steps, rules, and participants required to complete a specific task or business process.Each time a workflow definition is published, a new version is created. By default, the new version is used by existing instances of the workflow. If a stage is removed, active instances on the stage continue to use the previous version until they progress to a stage that is available in the newly published version.A 'Workflow Instance' is a specific occurrence of a workflow that is currently running or being executed, involving a series of interconnected computational tasks with data and control dependencies.  

### 2. Work Structure

A workflow can be structured at different levels, with individual work items grouped into larger stages or sub-processes to represent complex processes more clearly. Work items can also have dependencies, meaning one task may need to be completed before another can begin, while some tasks can run in parallel and later join together. During execution, work items can move through different states such as available, assigned, running, completed or suspended, so TIVIDY needs to track their current state, completed work and what can happen next. 

### 3. Assignment and Routing
Resources are modelled separately from the process, and a resource is anything capable of doing work, whether a person or a piece of equipment. People sit in an organisational structure of positions, units and teams, and have roles, capabilities, schedules and histories (Russell et al., 2004). Roles separate who does the work from the process definition. A task names a role such as Manager, and the person who fills it is only chosen when the work item becomes runnable, so one definition can serve many instances with different people (Russell et al., 2004). Allocation strategies are also kept separate from the work itself. Some are fixed at design time, such as role-based, capability-based, history-based and separation of duties, where the person who prepares a check cannot countersign it. Others select among eligible people, for example at random, round robin or shortest queue, and allocation can happen early, when the item becomes enabled, or late (Russell et al., 2004). A system can offer an item to one resource or to many, where the first to claim it wins, or allocate it directly. Under push the system assigns the item, and under pull the resource claims it. A work item moves from created to offered, allocated, started and completed, with suspended and failed as exceptions, and each change is started either by the system or by the resource. Assignment can also change through delegation, deallocation and escalation, where a stalled item is automatically reassigned after a deadline passes, and some tasks are automatic and need no person at all (Russell et al., 2004). For TIVIDY, this means roles, allocation rules and escalation deadlines belong to the workflow definition, so they can change without rewriting the work-item classes. The specific participants assigned, their workload and the assignment history belong to each running instance.

### 4. Approvals, Rejection and Escalation

Approval is a normal state in a workflow, not an error: the workflow pauses until a decision arrives and then follows the matching outcome of approval, rejection or escalation (Temporal Technologies, 2026). Approval rules vary between processes. Steps may be approved sequentially or in parallel, and a step may need every approver, a quorum, or any one approver, while a single rejection can veto the whole step (django-workflow-kit, n.d.). Rejection stops the request from reaching later approval steps, and the approver must record a reason that stays visible on the request (ServiceNow, n.d.). Escalation stops work from waiting indefinitely. If no decision arrives before a timeout, the workflow re-notifies the approver, then notifies their manager, and finally applies a default decision. Approvers can also delegate when unavailable (EmpowerID, n.d.; ServiceNow, n.d.). For TIVIDY, this means approval modes, timeouts and escalation chains belong to the workflow definition, so they can change without rewriting the work-item classes. The approval status, decision reason and escalation history belong to each running instance.

### 5. Events and Failures

 A workflow system must react to events such as completed tasks, expired deadlines and errors, which Russell, van der Aalst and ter Hofstede (2006) group into five exception types: work item failure, deadline expiry, resource unavailability, external triggers and constraint violations. They show that each exception must be handled at three levels: the individual work item, the whole case, and a recovery action such as rollback or compensation. Modern engines put this into practice by retrying temporary failures automatically, sending business errors down an alternative path, and raising an incident for a person to fix once retries run out, so that no failure goes unnoticed (Camunda, 2025). For TIVIDY, this means failure-handling rules belong to the workflow definition, while retry attempts, actual errors and open incidents belong to each running instance.


### 6. Integration and Reusability
Workflow systems keep the engine separate from external systems through defined interfaces: the WfMC Reference Model has dedicated interfaces for invoking external applications and for communicating with other engines (Workflow Management Coalition, 1995), and Camunda uses outbound connectors to call external systems and inbound connectors to bring events in (Camunda, 2025). Integration details are configuration, not code: Power Automate's connection references let one connection be reused across flows and environments without editing each action (Microsoft, 2024). For TIVIDY, this means a generic integration abstraction (databases, APIs, notifications) with configurable endpoints, and routing and assignment rules supplied per workflow, so our scenario is one application of a reusable engine.

### 7. History, Monitoring and Versioning
Workflow history serves as the chronological, event driven, append only log of all runtime events including state transitions across task lifecycles which provides the foundation for durable execution, crash recovery, and state replay. As a core component of history, audit information captures standardized system events, participant interactions, and execution data using the  prefix/suffix framework for legal compliance and accountability, extracted via push/pull ETL pipelines into structured relational tables while maintaining a strict physical separation between volatile runtime databases and immutable history streams to prevent performance degradation. 

Parallel to history, workflow versioning manages process blueprint evolution by enforcing in flight instance isolation guaranteeing that active, long running workflow instances complete safely on their originating definition version while new executions launch on the updated version, thereby preventing schema mismatches and preserving audit trail integrity. Finally, workflow monitoring provides the real time operational analysis and visual representation of active process instances during runtime, empowering administrators to detect bottlenecks, adjust running instance behavior dynamically, and enhance organizational responsiveness when handling customer status inquiries (such as determining who is processing a specific work item).

### References

Russell, N., ter Hofstede, A.H.M., Edmond, D. and van der Aalst, W.M.P., 2004. Workflow Resource Patterns. Technische Universiteit Eindhoven. Available at: https://pure.tue.nl/ws/files/1984893/591788.pdf
Workflow Management Coalition (1998). The Workflow Reference Model. http://www.workflowpatterns.com/documentation/documents/tc003v11.pdf 
Camunda. Glossary, Camunda 8 Docs. https://docs.camunda.io/docs/reference/glossary 
Microsoft. Use connector actions in desktop flows, Microsoft Learn. https://learn.microsoft.com/en-us/power-automate/desktop-flows/how-to/use-connector-actions

Zur Muehlen, M., 2004. Workflow-based process controlling: foundation, design, and application of workflow-driven process information systems (Vol. 6). Michael zur Muehlen.
Schiefer, J., Jeng, J.J. and Bruckner, R.M., 2003, September. Real-time workflow audit data integration into data warehouse systems. In ECIS (pp. 1697-1706).
Koksal, P., Arpinar, S.N. and Dogac, A., 1998. Workflow history management. ACM Sigmod Record, 27(1), pp.67-75.
Rolfe, P.A., 2001. Code versioning in a workflow management system (Doctoral dissertation, Massachusetts Institute of Technology).
https://docs.camunda.io/docs/components/best-practices/operations/versioning-process-definitions/


Camunda (2025) Dealing with problems and exceptions. Camunda 8 Documentation. Available at: https://docs.camunda.io/docs/components/best-practices/development/dealing-with-problems-and-exceptions/

Russell, N., van der Aalst, W.M.P. and ter Hofstede, A.H.M. (2006) 'Workflow exception patterns', in Advanced Information Systems Engineering (CAiSE 2006). Lecture Notes in Computer Science, vol. 4001. Berlin: Springer, pp. 288–302. Available at: https://doi.org/10.1007/11767138_20