#pragma once
/**
 * @file WorkflowEvent.h
 * @brief Event data pushed from the subject to observers (Observer) and sent to the Mediator.
 */
#include "tividy/core/Common.h"

class WorkflowInstance;
class WorkflowCommand;

/**
 * @brief What happened, where, and why.
 *
 * Pushed to every observer through update(e). @ref source lets an observer
 * reach its instance, and @ref cause gives the IncidentMonitor the command
 * to retry.
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

    WorkflowEvent() : type(EVT_CREATED), source(0), cause(0) {}

    /**
     * @param t Event type.
     * @param src Instance raising the event.
     * @param item Work item involved.
     * @param c Command that caused it.
     * @param info Extra details.
     */
    WorkflowEvent(EventType t, WorkflowInstance* src, const std::string& item = "",
                  WorkflowCommand* c = 0, const std::string& info = "")
        : type(t), workItemId(item), details(info), source(src), cause(c) {}
};
