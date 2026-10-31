#pragma once
#include <string>
#include "tividy/state/WorkItemState.h"

class WorkItem;

class CreatedState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    void makeAvailable(WorkItem&);
    void cancel(WorkItem&);
    void skip(WorkItem&);       // guard false or dead path: Created -> Skipped
    bool isCreated() const { return true; }     // WorkItem::isPending()
};

class AvailableState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    void assign(WorkItem&);
    void cancel(WorkItem&);
    bool isAvailable() const { return true; }   // WorkItem::isAvailable()
};

class AssignedState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    void assign(WorkItem&);   // AMENDMENT: reassignment stays Assigned
    void start(WorkItem&);
    void cancel(WorkItem&);
    void escalate(WorkItem&);
};

class InProgressState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    void complete(WorkItem&);
    void reject(WorkItem&);
    void cancel(WorkItem&);
    void suspend(WorkItem&);
};

class SuspendedState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    void cancel(WorkItem&);   // AMENDMENT: allow cancel from Suspended
    void resume(WorkItem&);
};

class EscalatedState : public WorkItemState {
public:
    Disposition disposition() const { return BLOCKED; }
    std::string name() const;
    static WorkItemState* instance();
    void start(WorkItem&);
    void complete(WorkItem&);
    void cancel(WorkItem&);
    void assign(WorkItem&);     // handed to the next level/supervisor: Escalated -> Assigned
};

class SkippedState : public WorkItemState {
public:
    Disposition disposition() const { return NOT_NEEDED; }
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};

class CompletedState : public WorkItemState {
public:
    Disposition disposition() const { return DONE; }
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};

class RejectedState : public WorkItemState {
public:
    Disposition disposition() const { return BLOCKED; }
    bool isRejection() const { return true; }
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};

class CancelledState : public WorkItemState {
public:
    Disposition disposition() const { return BLOCKED; }
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};