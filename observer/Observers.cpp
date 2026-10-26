/**
 * @file Observers.cpp
 * @brief Implementation of the concrete observers.
 */
#include "observer/Observers.h"
#include "command/Command.h"
#include "command/CommandProcessor.h"
#include "adapter/ExternalConnector.h"

void AuditLogger::update(const WorkflowEvent& e) {
    trail.push_back(e);
}

std::vector<std::string> AuditLogger::lines() const {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < trail.size(); ++i) {
        const WorkflowEvent& e = trail[i];
        std::string line = eventName(e.getType());
        if (!e.getWorkItemId().empty()) line += " " + e.getWorkItemId();
        if (!e.get("command").empty()) line += " [" + e.get("command") + "]";
        if (!e.get("message").empty()) line += " - " + e.get("message");
        out.push_back(line);
    }
    return out;
}

void NotificationService::update(const WorkflowEvent& e) {
    // Only events where a person has to do something.
    if (e.getType() != EventType::ASSIGNED && e.getType() != EventType::ESCALATED && e.getType() != EventType::REJECTED)
        return;

    std::string message = eventName(e.getType()) + ": " + e.getWorkItemId();
    if (!e.get("message").empty()) message += " (" + e.get("message") + ")";

    // Generic "send" request; the adapter translates it for the real SMS/email system.
    if (channel) {
        ConnectorRequest request;
        request.operation = "send";
        request.params["workItem"] = e.getWorkItemId();
        request.params["event"] = eventName(e.getType());
        request.params["message"] = message;
        channel->execute(request);
    }
    sent.push_back(message);
}

void IncidentMonitor::update(const WorkflowEvent& e) {
    if (e.getType() == EventType::TIMED_OUT) {
        raiseIncident(e, "deadline passed");
        return;
    }
    if (e.getType() != EventType::FAILED) return;

    // Without the causing command there is nothing to retry.
    if (!e.getCause() || !processor) {
        raiseIncident(e, "failure with no command to retry");
        return;
    }
    // Business errors take another path: retrying would fail the same way.
    if (!e.getCause()->isRetryable()) {
        raiseIncident(e, "business error, not retryable");
        return;
    }
    if (e.getCause()->getAttempts() >= maxAttempts) {
        raiseIncident(e, "retries exhausted");
        return;
    }
    // Temporary failure: try again. If it fails again, a new FAILED event brings us back here.
    processor->retry(e.getCause());
}

void IncidentMonitor::raiseIncident(const WorkflowEvent& e, const std::string& reason) {
    std::string text = "INCIDENT " + e.getWorkItemId() + ": " + reason;
    if (!e.get("message").empty()) text += " (" + e.get("message") + ")";
    incidents.push_back(text);
}
