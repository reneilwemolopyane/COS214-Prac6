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
    virtual EventType successEvent() const { return EventType::COMPLETED; }

    /** @brief Whether the last failure was temporary and worth retrying. */
    virtual bool isRetryable() const { return false; }

    /** @brief Whether undo() can reverse this command. Compensation escalates commands that cannot be undone. */
    virtual bool isReversible() const { return true; }

    /** @brief Whether compensation should touch this command at all. False for actions with nothing to undo, e.g. a sent notification. */
    virtual bool needsCompensation() const { return true; }

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
    EventType successEvent() const { return EventType::ASSIGNED; }
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
 * Receiver: ExternalConnector (Adapter pattern Target).
 *
 * The undo is not assumed: each external operation has its own reverse
 * operation and its own reference in the result, for example
 *   - "pay"   returns data["reference"]  -> undo "reverse"
 *   - "book"  returns data["bookingRef"] -> undo "cancel"
 *   - "store" returns data["documentId"] -> undo "delete"
 * The caller passes the undo operation. On success the whole result data is
 * kept, and undo() sends the undo operation with the original parameters plus
 * that result data, so whatever reference the adapter returned is passed back.
 * With no undo operation the command is not reversible.
 */
class InvokeServiceCommand : public WorkflowCommand {
public:
    /**
     * @param connector External system to call (not owned).
     * @param request Generic request, e.g. operation "pay" with an amount.
     * @param workItemId Work item this call belongs to.
     * @param undoOperation Operation that reverses this call (e.g. "reverse", "cancel", "delete"); empty if it cannot be undone.
     * @param issuedBy Who requested the call.
     */
    InvokeServiceCommand(ExternalConnector* connector, const ConnectorRequest& request,
                         const std::string& workItemId = "", const std::string& undoOperation = "",
                         const std::string& issuedBy = "system");
    bool execute();
    /** @brief Sends the undo operation with the original parameters plus the saved result data. */
    void undo();
    std::string name() const { return "InvokeService"; }
    bool isRetryable() const { return retryable; }
    bool isReversible() const { return !undoOperation.empty(); }
    /** @brief Data returned by the external system on success (e.g. its reference). */
    const DataMap& getResultData() const { return resultData; }
private:
    ExternalConnector* connector;   ///< Receiver (not owned).
    ConnectorRequest request;       ///< The original call.
    std::string undoOperation;      ///< Operation that reverses the call, empty if none.
    DataMap resultData;             ///< Result data from the successful call, used to build the undo.
    bool retryable;                 ///< Result of the last failed call.
};

/**
 * @brief ConcreteCommand: sends a message through a messaging connector.
 *
 * Receiver: ExternalConnector (e.g. the Adapter's MessagingAdapter).
 * Running a notification as a command means a failed send is retried by the
 * IncidentMonitor and recorded in the audit trail like any other action.
 * A sent message cannot be unsent and does not need to be, so compensation skips it.
 */
class SendNotificationCommand : public InvokeServiceCommand {
public:
    /**
     * @param channel Messaging connector (not owned).
     * @param request "send" request with "to", "text" and "urgent".
     * @param workItemId Work item the message is about.
     */
    SendNotificationCommand(ExternalConnector* channel, const ConnectorRequest& request,
                            const std::string& workItemId = "")
        : InvokeServiceCommand(channel, request, workItemId) {}
    std::string name() const { return "SendNotification"; }
    EventType successEvent() const { return EventType::NOTIFIED; }
    bool needsCompensation() const { return false; }
};