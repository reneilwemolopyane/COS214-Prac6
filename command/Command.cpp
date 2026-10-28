/**
 * @file Command.cpp
 * @brief Implementation of the concrete commands.
 */
#include "tividy/command/Command.h"
#include "tividy/composite/WorkItem.h"
#include "tividy/core/Participant.h"
#include "tividy/adapter/ExternalConnector.h"

AssignWorkItemCommand::AssignWorkItemCommand(WorkItem* item, Participant* assignee, const std::string& issuedBy)
    : WorkflowCommand(item ? item->getName() : "", issuedBy), item(item), assignee(assignee), previous(0) {}

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

CompleteWorkItemCommand::CompleteWorkItemCommand(WorkItem* item, const std::string& outcome, const std::string& issuedBy)
    : WorkflowCommand(item ? item->getName() : "", issuedBy), item(item), outcome(outcome) {}

bool CompleteWorkItemCommand::execute() {
    if (!item) {
        error = "missing work item";
        return false;
    }
    item->complete(outcome);
    return item->getOutcome() == outcome;
}

void CompleteWorkItemCommand::undo() {
    if (item) item->reopen();
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
    DataMap::const_iterator ref = result.data.find("referenceId");
    referenceId = (ref != result.data.end()) ? ref->second : "";
    return true;
}

void InvokeServiceCommand::undo() {
    if (!connector) return;
    // Same parameters as the original call, plus the reference id, under the undo operation.
    ConnectorRequest reverse;
    reverse.operation = undoOperation;
    reverse.params = request.params;
    reverse.params["referenceId"] = referenceId;
    connector->execute(reverse);
}
