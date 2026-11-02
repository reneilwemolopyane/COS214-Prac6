#pragma once
#include <string>
#include "tividy/state/WorkItemState.h"

class WorkItem;

/** @brief Initial state; items can become available, be cancelled, or be skipped. */
class CreatedState : public WorkItemState {
public:
    /** @return The state name, "Created". */
    std::string name() const;
    /** @return The shared Created state instance. */
    static WorkItemState* instance();
    /** @brief Transitions the item to Available. */
    void makeAvailable(WorkItem&);
    /** @brief Transitions the item to Cancelled. */
    void cancel(WorkItem&);
    void skip(WorkItem&);       // guard false or dead path: Created -> Skipped
    bool isCreated() const { return true; }     // WorkItem::isPending()
};

/** @brief State for an item ready to be assigned or cancelled. */
class AvailableState : public WorkItemState {
public:
    /** @return The state name, "Available". */
    std::string name() const;
    /** @return The shared Available state instance. */
    static WorkItemState* instance();
    /** @brief Transitions the item to Assigned. */
    void assign(WorkItem&);
    /** @brief Transitions the item to Cancelled. */
    void cancel(WorkItem&);
    bool isAvailable() const { return true; }   // WorkItem::isAvailable()
};

/** @brief Assigned items can start, be reassigned, escalated, or cancelled. */
class AssignedState : public WorkItemState {
public:
    /** @return The state name, "Assigned". */
    std::string name() const;
    /** @return The shared Assigned state instance. */
    static WorkItemState* instance();
    void assign(WorkItem&);   // AMENDMENT: reassignment stays Assigned
    /** @brief Transitions the item to InProgress. */
    void start(WorkItem&);
    /** @brief Transitions the item to Cancelled. */
    void cancel(WorkItem&);
    /** @brief Transitions the item to Escalated. */
    void escalate(WorkItem&);
};

/** @brief State for active work; supports completion, rejection, cancellation, and suspension. */
class InProgressState : public WorkItemState {
public:
    /** @return The state name, "InProgress". */
    std::string name() const;
    /** @return The shared InProgress state instance. */
    static WorkItemState* instance();
    /** @brief Transitions the item to Completed. */
    void complete(WorkItem&);
    /** @brief Transitions the item to Rejected. */
    void reject(WorkItem&);
    /** @brief Transitions the item to Cancelled. */
    void cancel(WorkItem&);
    /** @brief Transitions the item to Suspended. */
    void suspend(WorkItem&);
};

/** @brief Suspended work can be resumed or cancelled. */
class SuspendedState : public WorkItemState {
public:
    /** @return The state name, "Suspended". */
    std::string name() const;
    /** @return The shared Suspended state instance. */
    static WorkItemState* instance();
    void cancel(WorkItem&);   // AMENDMENT: allow cancel from Suspended
    /** @brief Transitions the item back to InProgress. */
    void resume(WorkItem&);
};

/** @brief Escalated state; the item is blocked until reassigned or otherwise resolved. */
class EscalatedState : public WorkItemState {
public:
    /** @return BLOCKED. */
    Disposition disposition() const { return BLOCKED; }
    /** @return The state name, "Escalated". */
    std::string name() const;
    /** @return The shared Escalated state instance. */
    static WorkItemState* instance();
    void start(WorkItem&);
    void complete(WorkItem&);
    void cancel(WorkItem&);
    void assign(WorkItem&);     // handed to the next level/supervisor: Escalated -> Assigned
};

/** @brief Terminal state for a branch that was not needed. */
class SkippedState : public WorkItemState {
public:
    /** @return NOT_NEEDED. */
    Disposition disposition() const { return NOT_NEEDED; }
    /** @return The state name, "Skipped". */
    std::string name() const;
    /** @return The shared Skipped state instance. */
    static WorkItemState* instance();
    /** @return true because Skipped is terminal. */
    bool isFinal() const { return true; }
};

/** @brief Successful terminal state. */
class CompletedState : public WorkItemState {
public:
    /** @return DONE. */
    Disposition disposition() const { return DONE; }
    /** @return The state name, "Completed". */
    std::string name() const;
    /** @return The shared Completed state instance. */
    static WorkItemState* instance();
    /** @return true because Completed is terminal. */
    bool isFinal() const { return true; }
};

/** @brief Terminal state for work that was rejected. */
class RejectedState : public WorkItemState {
public:
    /** @return BLOCKED. */
    Disposition disposition() const { return BLOCKED; }
    /** @return true. */
    bool isRejection() const { return true; }
    /** @return The state name, "Rejected". */
    std::string name() const;
    /** @return The shared Rejected state instance. */
    static WorkItemState* instance();
    /** @return true because Rejected is terminal. */
    bool isFinal() const { return true; }
};

/** @brief Terminal state for cancelled work. */
class CancelledState : public WorkItemState {
public:
    /** @return BLOCKED. */
    Disposition disposition() const { return BLOCKED; }
    /** @return The state name, "Cancelled". */
    std::string name() const;
    /** @return The shared Cancelled state instance. */
    static WorkItemState* instance();
    /** @return true because Cancelled is terminal. */
    bool isFinal() const { return true; }
};