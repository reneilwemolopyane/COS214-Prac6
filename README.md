# COS214-Prac6

## Team Members

| Student Name and Surname | Student Number |
|---|---|
| Lizalise Mbonisweni | u23587874 |
| Boikemelo Masoka | u25128648 |
| Thembelisha Skosana | u25224663 |
| Lethabo Molobi |  |
| Boitumelo Monareng |  |
| Reneilwe Molopyane | u25161874 |
| Dingalethu Ngumbela | u25170547 |

## Task 1: Research

### 1. Workflow Basics: Definition vs Instance

A Workflow Management System is a system that defines, manages, and executes workflows using software based on the workflow logic. It automates the execution of applications and is robust against performance variations and failures.A workflow definition is a structured blueprint or electronic template that outlines the exact sequence of steps, rules, and participants required to complete a specific task or business process.Each time a workflow definition is published, a new version is created. By default, the new version is used by existing instances of the workflow. If a stage is removed, active instances on the stage continue to use the previous version until they progress to a stage that is available in the newly published version.A 'Workflow Instance' is a specific occurrence of a workflow that is currently running or being executed, involving a series of interconnected computational tasks with data and control dependencies.  

### 2. Work Structure

### 3. Assignment and Routing

### 4. Approvals, Rejection and Escalation

Approval is a normal state in a workflow, not an error: the workflow pauses until a decision arrives and then follows the matching outcome of approval, rejection or escalation (Temporal Technologies, 2026). Approval rules vary between processes. Steps may be approved sequentially or in parallel, and a step may need every approver, a quorum, or any one approver, while a single rejection can veto the whole step (django-workflow-kit, n.d.). Rejection stops the request from reaching later approval steps, and the approver must record a reason that stays visible on the request (ServiceNow, n.d.). Escalation stops work from waiting indefinitely. If no decision arrives before a timeout, the workflow re-notifies the approver, then notifies their manager, and finally applies a default decision. Approvers can also delegate when unavailable (EmpowerID, n.d.; ServiceNow, n.d.). For TIVIDY, this means approval modes, timeouts and escalation chains belong to the workflow definition, so they can change without rewriting the work-item classes. The approval status, decision reason and escalation history belong to each running instance.

### 5. Events and Failures

 A workflow system has to react to events such as a task finishing, a deadline passing, a message arriving or an error occurring. Russell, van der Aalst and ter Hofstede (2006) identify five types of exception: work item failure, deadline expiry, resource unavailability, external triggers and constraint violations. Each must be handled at the work-item level, at the whole-case level and with a recovery action such as rollback or compensation. Planning ahead matters, because unexpected exceptions slow processes down significantly more than expected ones (Dijkman et al., 2019). Modern engines therefore build failure handling into the workflow definition: temporary failures are retried automatically, business errors are sent down an alternative path, completed work can be undone through compensation, and failures that still can't be resolved raise an incident for a person to fix (Camunda, 2025; Temporal Technologies, 2026). Handlers can even be chosen at runtime based on the situation (Adams et al., 2007). For TIVIDY, this means failure rules belong to the workflow definition, while retry attempts, actual errors and open incidents belong to each running instance.


### 6. Integration and Reusability

### 7. History, Monitoring and Versioning
Workflow history serves as the chronological, event-driven, append-only log of all runtime events—including state transitions across task lifecycles—which provides the foundation for durable execution, crash recovery, and state replay. As a core component of history, audit information captures standardized system events, participant interactions, and execution data using the WfMC Interface 5 Common Workflow Audit Data (CWAD) prefix/suffix framework for legal compliance and accountability, extracted via push/pull ETL pipelines into structured relational tables while maintaining a strict physical separation between volatile runtime databases and immutable history streams to prevent performance degradation. Parallel to history, workflow versioning manages process blueprint evolution by enforcing in-flight instance isolation—guaranteeing that active, long-running workflow instances complete safely on their originating definition version while new executions launch on the updated version, thereby preventing schema mismatches and preserving audit trail integrity. Finally, workflow monitoring provides the real-time operational analysis and visual representation of active process instances during runtime, empowering administrators to detect bottlenecks, adjust running instance behavior dynamically, and enhance organizational responsiveness when handling customer status inquiries (such as determining who is processing a specific work item).

### References

