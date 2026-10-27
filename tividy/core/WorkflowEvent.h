#pragma once
/**
 * @file WorkflowEvent.h
 * @brief Immutable event passed to observers (Observer) and to the Mediator.
 */
#include <string>
#include "tividy/core/Common.h"

class WorkflowInstance;
class WorkflowCommand;

/**
 * @brief What happened, to which work item, plus extra data.
 *
 * Immutable: everything is set in the constructor and only read through getters.
 * Extra information goes in the payload, e.g. payload["message"] for an error,
 * payload["assignee"] for an assignment, payload["command"] for the causing command.
 *
 * getCause() gives the IncidentMonitor the command to retry (null if not caused by a command).
 * getSource() is the instance that raised it (null if raised outside an instance).
 */
class WorkflowEvent {
public:
    /**
     * @param type What happened.
     * @param workItemId Work item involved, empty if none.
     * @param payload Extra key/value data.
     * @param cause Command that caused the event, or null (not owned).
     * @param source Instance that raised the event, or null (not owned).
     */
    explicit WorkflowEvent(EventType type, const std::string& workItemId = "",
                           const DataMap& payload = DataMap(),
                           WorkflowCommand* cause = 0, WorkflowInstance* source = 0)
        : type(type), workItemId(workItemId), payload(payload), cause(cause), source(source) {}

    EventType getType() const { return type; }
    const std::string& getWorkItemId() const { return workItemId; }
    const DataMap& getPayload() const { return payload; }

    /** @brief Payload value for @p key, or "" if missing. */
    std::string get(const std::string& key) const {
        DataMap::const_iterator it = payload.find(key);
        return it == payload.end() ? "" : it->second;
    }

    WorkflowCommand* getCause() const { return cause; }
    WorkflowInstance* getSource() const { return source; }

private:
    EventType type;
    std::string workItemId;
    DataMap payload;
    WorkflowCommand* cause;
    WorkflowInstance* source;
};