#pragma once
/**
 * @file Command.h
 * @brief Command pattern: WorkflowCommand and its concrete commands.
 *
 * Each workflow action is wrapped in an object that knows how to do itself
 * (execute) and how to reverse itself (undo), so the CommandProcessor can
 * record, retry and compensate actions without knowing what they do.
 */
#include <string>
#include "tividy/core/Common.h"
#include "tividy/adapter/ConnectorResult.h"

class WorkItem;
class Participant;
class ExternalConnector;

/**
 * @brief Command pattern abstract Command.
 *
 * Stores the metadata needed for the audit trail and for retries.
 * Status and attempts are only changed by the CommandProcessor (Invoker).
 */
class WorkflowCommand {
    friend class CommandProcessor;   ///< Invoker updates status and attempts.
public:
    /**
     * @param workItemId Work item this action applies to, used in events.
     * @param issuedBy Who requested the action.
     */
    explicit WorkflowCommand(const std::string& workItemId = "", const std::string& issuedBy = "system")
        : workItemId(workItemId), issuedBy(issuedBy), status("PENDING"), attempts(0) {}
    virtual ~WorkflowCommand() {}

    /** @brief Performs the action on the receiver. @return true on success. */
    virtual bool execute() = 0;

    /** @brief Reverses a successful execute(). */
    virtual void undo() = 0;

    /** @brief Short name used in events and the audit trail. */
    virtual std::string name() const = 0;

    /** @brief Event raised when execute() succeeds. */
    virtual EventType successEvent() const { return EVT_COMPLETED; }

    /** @brief Whether the last failure was temporary and worth retrying. */
    virtual bool isRetryable() const { return false; }

    std::string getWorkItemId() const { return workItemId; }
    std::string getIssuedBy() const { return issuedBy; }

    /** @brief PENDING, DONE, FAILED or UNDONE. */
    std::string getStatus() const { return status; }

    /** @brief Reason for the last failure, empty if none. */
    std::string getError() const { return error; }

    /** @brief Number of times execute() has been attempted. */
    int getAttempts() const { return attempts; }

protected:
    std::string workItemId, issuedBy, status, error;
    int attempts;

private:
    WorkflowCommand(const WorkflowCommand&);              ///< Not copyable.
    WorkflowCommand& operator=(const WorkflowCommand&);   ///< Not assignable.
};

/**
 * @brief ConcreteCommand: assigns a work item to a participant.
 *
 * Receiver: WorkItem. Saves the previous assignee so undo() can restore it.
 */
class AssignWorkItemCommand : public WorkflowCommand {
public:
    AssignWorkItemCommand(WorkItem* item, Participant* assignee, const std::string& issuedBy = "system");
    bool execute();
    /** @brief Gives the item back to the previous assignee (or nobody). */
    void undo();
    std::string name() const { return "AssignWorkItem"; }
    EventType successEvent() const { return EVT_ASSIGNED; }
private:
    WorkItem* item;          ///< Receiver (not owned).
    Participant* assignee;   ///< New assignee (not owned).
    Participant* previous;   ///< Saved for undo.
};

/**
 * @brief ConcreteCommand: completes a work item with an outcome.
 *
 * Receiver: WorkItem. undo() reopens the item.
 */
class CompleteWorkItemCommand : public WorkflowCommand {
public:
    CompleteWorkItemCommand(WorkItem* item, const std::string& outcome, const std::string& issuedBy = "system");
    bool execute();
    /** @brief Reopens the work item. */
    void undo();
    std::string name() const { return "CompleteWorkItem"; }
private:
    WorkItem* item;        ///< Receiver (not owned).
    std::string outcome;   ///< e.g. "approved".
};

/**
 * @brief ConcreteCommand: calls an external system through an ExternalConnector.
 *
 * Receiver: ExternalConnector (Adapter pattern Target), e.g. a payment adapter.
 * Keeps the reference id returned by the system so undo() can reverse the
 * call (e.g. refund a payment). The connector reports whether a failure is
 * temporary, which decides if the IncidentMonitor retries it.
 */
class InvokeServiceCommand : public WorkflowCommand {
public:
    /**
     * @param connector External system to call (not owned).
     * @param request Generic request, e.g. operation "pay" with an amount.
     * @param workItemId Work item this call belongs to.
     * @param undoOperation Operation sent on undo, "reverse" by default.
     * @param issuedBy Who requested the call.
     */
    InvokeServiceCommand(ExternalConnector* connector, const ConnectorRequest& request,
                         const std::string& workItemId = "", const std::string& undoOperation = "reverse",
                         const std::string& issuedBy = "system");
    bool execute();
    /** @brief Sends the undo operation with the saved reference id. */
    void undo();
    std::string name() const { return "InvokeService"; }
    bool isRetryable() const { return retryable; }
    /** @brief Id returned by the external system on success. */
    std::string getReferenceId() const { return referenceId; }
private:
    ExternalConnector* connector;   ///< Receiver (not owned).
    ConnectorRequest request;       ///< The original call.
    std::string undoOperation;      ///< Operation used to reverse the call.
    std::string referenceId;        ///< Saved for undo.
    bool retryable;                 ///< Result of the last failed call.
};
