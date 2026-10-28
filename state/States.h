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
};

class AvailableState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    void assign(WorkItem&);
    void cancel(WorkItem&);
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
    std::string name() const;
    static WorkItemState* instance();
    void start(WorkItem&);
    void complete(WorkItem&);
    void cancel(WorkItem&);
};

class CompletedState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};

class RejectedState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};

class CancelledState : public WorkItemState {
public:
    std::string name() const;
    static WorkItemState* instance();
    bool isFinal() const { return true; }
};