/**
 * @file Command.cpp
 * @brief Implementation of the concrete commands.
 */
#include "command/Command.h"
#include "composite_prototype/WorkItem.h"
#include "core/Participant.h"
#include "adapter/ExternalConnector.h"
#include <stdexcept>

AssignWorkItemCommand::AssignWorkItemCommand(WorkItem* item, Participant* assignee, const std::string& issuedBy)
    : WorkflowCommand(item ? item->getName() : "", issuedBy), item(item), assignee(assignee), previous(0) {}

bool AssignWorkItemCommand::execute() {
    if (!item || !assignee) {
        error = "missing work item or participant";
        return false;
    }
    // Remember who had it before, so undo() can give it back.
    previous = item->getAssignee();
    try {
        item->assign(assignee);
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    return item->getAssignee() == assignee;
}

void AssignWorkItemCommand::undo() {
    if (!item) return;
    try {
        if (previous) item->assign(previous);
        else item->releaseAssignee();
    } catch (const std::exception& e) {
        error = e.what();
    }
}

CompleteWorkItemCommand::CompleteWorkItemCommand(WorkItem* item, const std::string& outcome, const std::string& issuedBy)
    : WorkflowCommand(item ? item->getName() : "", issuedBy), item(item), outcome(outcome) {}

bool CompleteWorkItemCommand::execute() {
    if (!item) {
        error = "missing work item";
        return false;
    }
    try {
        item->complete(outcome);
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
    return item->getOutcome() == outcome;
}

void CompleteWorkItemCommand::undo() {
    if (!item) return;
    try {
        item->reopen();
    } catch (const std::exception& e) {
        error = e.what();
    }
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
    // Keep everything the system returned; undo() needs its reference to reverse this exact call.
    resultData = result.data;
    return true;
}

void InvokeServiceCommand::undo() {
    if (!connector || undoOperation.empty()) return;
    // Original parameters plus the returned data (e.g. reference, bookingRef, documentId).
    ConnectorRequest reverse;
    reverse.operation = undoOperation;
    reverse.params = request.params;
    for (DataMap::const_iterator it = resultData.begin(); it != resultData.end(); ++it)
        reverse.params[it->first] = it->second;
    connector->execute(reverse);
}