#pragma once
/**
 * @file WorkflowEvent.h
 * @brief Event passed to observers (Observer) and to the Mediator.
 */
#include <string>
#include "tividy/core/Common.h"

class WorkflowInstance;
class WorkflowCommand;

/**
 * @brief What happened, to which work item, plus extra data.
 *
 * Two ways to build and read it, so every module can use the style it was written with:
 * - Fields: e.type, e.workItemId, e.details, e.source, e.cause,
 *   built with WorkflowEvent(type, source, workItemId, cause, details).
 * - Getters and payload: e.getType(), e.getWorkItemId(), e.get("message"),
 *   built with WorkflowEvent(type, workItemId, payload, cause, source).
 *
 * details and payload["message"] always hold the same text.
 * getCause() gives the IncidentMonitor the command to retry (null if not caused by a command).
 * getSource() is the instance that raised it (null if raised outside an instance).
 */
struct WorkflowEvent {
    EventType type;               ///< What happened.
    std::string instanceId;       ///< Id of the instance it happened in.
    std::string workItemId;       ///< Work item involved, if any.
    std::string timestamp;        ///< When it happened.
    std::string details;          ///< Extra information, e.g. an error message.
    std::string commandName;      ///< Name of the causing command, for the audit trail.
    WorkflowInstance* source;     ///< Instance that raised the event (not owned).
    WorkflowCommand* cause;       ///< Command that caused it, or null (not owned).
    DataMap payload;              ///< Extra key/value data, e.g. payload["assignee"].

    WorkflowEvent() : type(EVT_CREATED), source(0), cause(0) {}

    /**
     * @brief Field style.
     * @param t Event type.
     * @param src Instance raising the event.
     * @param item Work item involved.
     * @param c Command that caused it.
     * @param info Extra details, also stored as payload["message"].
     */
    WorkflowEvent(EventType t, WorkflowInstance* src, const std::string& item = "",
                  WorkflowCommand* c = 0, const std::string& info = "")
        : type(t), workItemId(item), details(info), source(src), cause(c) {
        if (!info.empty()) payload["message"] = info;
    }

    /**
     * @brief Payload style.
     * @param t What happened.
     * @param item Work item involved, empty if none.
     * @param data Extra key/value data; data["message"] also fills details.
     * @param c Command that caused the event, or null (not owned).
     * @param src Instance that raised the event, or null (not owned).
     */
    WorkflowEvent(EventType t, const std::string& item, const DataMap& data = DataMap(),
                  WorkflowCommand* c = 0, WorkflowInstance* src = 0)
        : type(t), workItemId(item), source(src), cause(c), payload(data) {
        details = get("message");
        instanceId = get("instance");
        commandName = get("command");
    }

    EventType getType() const { return type; }
    const std::string& getWorkItemId() const { return workItemId; }
    const DataMap& getPayload() const { return payload; }

    /** @brief Payload value for @p key, or "" if missing. "message" falls back to details. */
    std::string get(const std::string& key) const {
        DataMap::const_iterator it = payload.find(key);
        if (it != payload.end()) return it->second;
        return key == "message" ? details : "";
    }

    WorkflowCommand* getCause() const { return cause; }
    WorkflowInstance* getSource() const { return source; }
};
