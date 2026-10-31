/**
 * @file CommandProcessor.cpp
 * @brief Implementation of the Command pattern Invoker.
 */
#include "tividy/command/CommandProcessor.h"
#include "tividy/core/WorkflowInstance.h"
#include <algorithm>
#include <exception>

CommandProcessor::~CommandProcessor() {
    for (std::size_t i = 0; i < history.size(); ++i)
        delete history[i];
}

bool CommandProcessor::run(WorkflowCommand* command) {
    if (!command) return false;
    if (!owns(command)) history.push_back(command);
    return attempt(command);
}

bool CommandProcessor::retry(WorkflowCommand* command) {
    if (!command || !owns(command) || command->status != "FAILED") return false;
    return attempt(command);
}

void CommandProcessor::compensate() {
    // Newest first: undo the last thing that was done before the things it depended on.
    for (std::size_t i = history.size(); i > 0; --i) {
        WorkflowCommand* c = history[i - 1];
        if (c->status != "DONE") continue;   // Failed or already undone commands have nothing to reverse.
        c->undo();
        c->status = "UNDONE";
        if (instance) instance->raiseEvent(EVT_COMPENSATED, c->getWorkItemId(), c);
    }
}

bool CommandProcessor::attempt(WorkflowCommand* command) {
    ++command->attempts;
    bool ok = false;
    try { ok = command->execute(); }
    catch (const std::exception& e) {          // a refused action (invalid transition, decorator veto)
        command->error = e.what();
        command->status = "REFUSED";
        throw;                                  // the engine records REFUSED in the audit trail
    }
    command->status = ok ? "DONE" : "FAILED";
    if (instance) {
        if (ok) { if (command->announcesSuccess()) instance->raiseEvent(command->successEvent(), command->getWorkItemId(), command, command->describe()); }
        else    instance->raiseEvent(EVT_FAILED, command->getWorkItemId(), command, command->getError());
    }
    // A FAILED event may have made the IncidentMonitor retry this command already,
    // so return the final status rather than the result of this one attempt.
    return command->status == "DONE";
}

bool CommandProcessor::owns(WorkflowCommand* command) const {
    return std::find(history.begin(), history.end(), command) != history.end();
}
