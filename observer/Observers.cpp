/**
 * @file Observers.cpp
 * @brief Implementation of the concrete observers.
 */
#include "tividy/observer/Observers.h"
#include "tividy/command/Command.h"
#include "tividy/command/CommandProcessor.h"
#include "tividy/adapter/ExternalConnector.h"
#include <algorithm>

// ---- AuditLogger ----

void AuditLogger::update(const WorkflowEvent& event) {
    WorkflowEvent copy = event;
    copy.source = 0;   // The trail outlives nothing it points to.
    copy.cause = 0;
    trail.push_back(copy);
}

std::string AuditLogger::format(const WorkflowEvent& e) {
    std::string line = "[" + e.timestamp + "] " + eventTypeName(e.type);
    if (!e.workItemId.empty()) line += " " + e.workItemId;
    if (!e.commandName.empty()) line += " via " + e.commandName;
    if (!e.details.empty()) line += " (" + e.details + ")";
    return line;
}

std::vector<std::string> AuditLogger::getLines() const {
    std::vector<std::string> lines;
    for (std::size_t i = 0; i < trail.size(); ++i) lines.push_back(format(trail[i]));
    return lines;
}

// ---- DependencyTracker ----

namespace {
void addOnce(std::vector<std::string>& v, const std::string& id) {
    if (!id.empty() && std::find(v.begin(), v.end(), id) == v.end()) v.push_back(id);
}
}

void DependencyTracker::update(const WorkflowEvent& e) {
    switch (e.type) {
        case EVT_COMPLETED:
            addOnce(completed, e.workItemId);
            break;
        case EVT_FAILED:
            // A failed command may still be retried; only a failure with no command closes the item.
            if (!e.cause) addOnce(closed, e.workItemId);
            break;
        case EVT_REJECTED:
        case EVT_CANCELLED:
        case EVT_ESCALATED:
        case EVT_TIMED_OUT:
            addOnce(closed, e.workItemId);
            break;
        default:
            break;
    }
}

bool DependencyTracker::isCompleted(const std::string& id) const {
    return std::find(completed.begin(), completed.end(), id) != completed.end();
}

bool DependencyTracker::isClosed(const std::string& id) const {
    return std::find(closed.begin(), closed.end(), id) != closed.end();
}

// ---- NotificationService ----

void NotificationService::update(const WorkflowEvent& e) {
    if (e.type == EVT_ASSIGNED) {
        // The coordinator reports the assignee as "assignee: <name>".
        const std::string prefix = "assignee: ";
        std::string to = e.details.compare(0, prefix.size(), prefix) == 0
                             ? e.details.substr(prefix.size()) : e.details;
        if (to.empty()) return;   // Nobody to tell.
        send(to, "Work item '" + e.workItemId + "' is waiting for you.", false);
    } else if (e.type == EVT_ESCALATED) {
        send("supervisor", "Work item '" + e.workItemId + "' was escalated: " + e.details, true);
    }
}

void NotificationService::send(const std::string& to, const std::string& text, bool urgent) {
    messages.push_back(to + ": " + text);
    if (!connector) return;

    ConnectorRequest request;
    request.operation = "send";
    request.params["to"] = to;
    request.params["text"] = text;
    request.params["urgent"] = urgent ? "true" : "false";
    if (!connector->execute(request).success) ++failures;
}

// ---- IncidentMonitor ----

void IncidentMonitor::update(const WorkflowEvent& e) {
    if (e.type != EVT_FAILED && e.type != EVT_TIMED_OUT) return;

    WorkflowCommand* command = e.cause;
    if (processor && command && command->isRetryable() && command->getAttempts() < maxAttempts) {
        processor->retry(command);   // May raise more events (including another FAILED).
        return;
    }

    std::string what = command ? command->name() : std::string("work item");
    std::string incident = "Incident: " + what;
    if (!e.workItemId.empty()) incident += " on " + e.workItemId;
    if (command) incident += " failed after " + std::to_string(command->getAttempts()) + " attempt(s)";
    if (!e.details.empty()) incident += ": " + e.details;
    incidents.push_back(incident);
}
