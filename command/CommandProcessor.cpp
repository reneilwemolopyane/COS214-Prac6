/**
 * @file CommandProcessor.cpp
 * @brief Implementation of the Command pattern Invoker.
 */
#include "command/CommandProcessor.h"
#include "core/WorkflowInstance.h"
#include <algorithm>

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
        if (!c->isReversible()) {
            // Cannot be undone automatically: leave it DONE and escalate so a person handles it.
            if (instance) instance->raiseEvent(EventType::ESCALATED, c->getWorkItemId(), c, "cannot be undone automatically");
            continue;
        }
        c->undo();
        c->status = "UNDONE";
        if (instance) instance->raiseEvent(EventType::COMPENSATED, c->getWorkItemId(), c);
    }
}

bool CommandProcessor::attempt(WorkflowCommand* command) {
    ++command->attempts;
    bool ok = command->execute();
    command->status = ok ? "DONE" : "FAILED";
    if (instance) {
        if (ok) instance->raiseEvent(command->successEvent(), command->getWorkItemId(), command);
        else    instance->raiseEvent(EventType::FAILED, command->getWorkItemId(), command, command->getError());
    }
    // A FAILED event may have made the IncidentMonitor retry this command already,
    // so return the final status rather than the result of this one attempt.
    return command->status == "DONE";
}

bool CommandProcessor::owns(WorkflowCommand* command) const {
    return std::find(history.begin(), history.end(), command) != history.end();
}