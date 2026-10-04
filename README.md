# COS214-Prac6

<<<<<<< HEAD
## Team Members

| Student Name and Surname | Student Number |
|---|---|
| Lizalise Mbonisweni |  |
| Boikemelo Masoka | u25128648 |
| Thembelisha Skosana | u25224663 |
| Lethabo Molobi |  |
| Boitumelo Monareng |  |
| Reneilwe Molopyane | u25161874 |
| Dingalethu Ngumbela | u25170547 |

## Task 1: Research

### 1. Workflow Basics: Definition vs Instance

### 2. Work Structure

### 3. Assignment and Routing

### 4. Approvals, Rejection and Escalation

### 5. Events and Failures

 A workflow system has to react to events such as a task finishing, a deadline passing, a message arriving or an error occurring. Russell, van der Aalst and ter Hofstede (2006) identify five types of exception: work item failure, deadline expiry, resource unavailability, external triggers and constraint violations. Each must be handled at the work-item level, at the whole-case level and with a recovery action such as rollback or compensation. Planning ahead matters, because unexpected exceptions slow processes down significantly more than expected ones (Dijkman et al., 2019). Modern engines therefore build failure handling into the workflow definition: temporary failures are retried automatically, business errors are sent down an alternative path, completed work can be undone through compensation, and failures that still can't be resolved raise an incident for a person to fix (Camunda, 2025; Temporal Technologies, 2026). Handlers can even be chosen at runtime based on the situation (Adams et al., 2007). For TIVIDY, this means failure rules belong to the workflow definition, while retry attempts, actual errors and open incidents belong to each running instance.


### 6. Integration and Reusability

### 7. History, Monitoring and Versioning

### References
=======
Practical Research:

1. Theme 1:  
A Workflow Management System is a system that defines, manages, and executes workflows using software based on the workflow logic. It automates the execution of applications and is robust against performance variations and failures.A workflow definition is a structured blueprint or electronic template that outlines the exact sequence of steps, rules, and participants required to complete a specific task or business process.Each time a workflow definition is published, a new version is created. By default, the new version is used by existing instances of the workflow. If a stage is removed, active instances on the stage continue to use the previous version until they progress to a stage that is available in the newly published version.A 'Workflow Instance' is a specific occurrence of a workflow that is currently running or being executed, involving a series of interconnected computational tasks with data and control dependencies.  
>>>>>>> 0e854419eabdfbe8319915a2567191faa5a68220
