/**
 * @file Command.cpp
 * @brief Implementation of the concrete commands.
 */
#include "tividy/command/Command.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Participant.h"
#include "tividy/adapter/ExternalConnector.h"
#include <stdexcept>
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/composite_prototype/WorkflowComponent.h"

AssignWorkItemCommand::AssignWorkItemCommand(WorkItem* item, Participant* assignee, const std::string& issuedBy)
    : WorkflowCommand(item ? item->getId() : "", issuedBy), item(item), assignee(assignee), previous(0) {}

bool AssignWorkItemCommand::execute() {
    if (!item || !assignee) {
        error = "missing work item or participant";
        return false;
    }
    // Remember who had it before, so undo() can give it back.
    previous = item->getAssignee();
    item->assign(assignee);
    return item->getAssignee() == assignee;
}

void AssignWorkItemCommand::undo() {
    if (item) item->assign(previous);
}

CompleteWorkItemCommand::CompleteWorkItemCommand(WorkItem* item, const std::string& outcome, const std::string& issuedBy,
                                                 WorkflowComponent* front)
    : WorkflowCommand(item ? item->getId() : "", issuedBy), item(item), outcome(outcome), front(front) {}

bool CompleteWorkItemCommand::execute() {
    if (!item) {
        error = "missing work item";
        return false;
    }
    if (front) front->complete(outcome);   // through the decorators
    else item->complete(outcome);
    return item->getOutcome() == outcome;
}

void CompleteWorkItemCommand::undo() {
    if (item) item->reopen();
}

StartWorkItemCommand::StartWorkItemCommand(WorkItem* item, const std::string& issuedBy, WorkflowComponent* front)
    : WorkflowCommand(item ? item->getId() : "", issuedBy), item(item), front(front) {}

bool StartWorkItemCommand::execute() {
    if (!item) { error = "missing work item"; return false; }
    if (front) front->start(); else item->start();
    return true;
}

RejectWorkItemCommand::RejectWorkItemCommand(WorkItem* item, const std::string& reason, const std::string& issuedBy)
    : WorkflowCommand(item ? item->getId() : "", issuedBy), item(item), reason(reason) {}

bool RejectWorkItemCommand::execute() {
    if (!item) { error = "missing work item"; return false; }
    item->reject(reason);
    return true;
}

EscalateWorkItemCommand::EscalateWorkItemCommand(InstanceCoordinator* coordinator, const std::string& itemId,
                                                 const std::string& reason, const std::string& issuedBy)
    : WorkflowCommand(itemId, issuedBy), coordinator(coordinator), reason(reason) {}

bool EscalateWorkItemCommand::execute() {
    if (!coordinator) { error = "no coordinator"; return false; }
    // Nobody higher is a refusal of the request, not a system failure: throw so the engine records REFUSED
    // (a FAILED event would make the IncidentMonitor raise a false incident).
    if (!coordinator->escalate(workItemId, reason)) throw std::runtime_error("nobody higher to escalate " + workItemId + " to");
    return true;
}

InvokeServiceCommand::InvokeServiceCommand(ExternalConnector* connector, const ConnectorRequest& request,
                                           const std::string& workItemId, const std::string& undoOperation,
                                           const std::string& issuedBy)
    : WorkflowCommand(workItemId, issuedBy), connector(connector), request(request),
      undoOperation(undoOperation), retryable(false) {}

bool InvokeServiceCommand::execute() {
    if (!connector) {
        error = "missing connector";
        retryable = false;
        return false;
    }
    ConnectorResult result = connector->execute(request);
    // The adapter decides whether the failure is temporary (e.g. system down) or a business error.
    retryable = result.retryable;
    if (!result.success) {
        error = result.message;
        return false;
    }
    error.clear();
    // Keep the system's reference id; undo() needs it to reverse this exact call.
    resultData = result.data;
    resultMessage = result.message;
    DataMap::const_iterator ref = result.data.find("reference");
    referenceId = (ref != result.data.end()) ? ref->second : "";
    return true;
}

void InvokeServiceCommand::undo() {
    if (!connector) return;
    // Same parameters as the original call, plus the reference id, under the undo operation.
    ConnectorRequest reverse;
    reverse.operation = undoOperation;
    reverse.params = request.params;
    reverse.params["reference"] = referenceId;   // key the adapters use
    connector->execute(reverse);
}

std::string InvokeServiceCommand::describe() const {
    std::string text = connector ? connector->name() + ": " + resultMessage : resultMessage;
    for (ParamMap::const_iterator it = resultData.begin(); it != resultData.end(); ++it)
        text += (it == resultData.begin() ? " [" : ", ") + it->first + "=" + it->second;
    if (!resultData.empty()) text += "]";
    return text;
}
