#pragma once
/** @file WorkItemState.h  State interface. STUB: TODO default refuse behaviour. */
#include <string>
class WorkItem;
class WorkItemState {
public:
    virtual ~WorkItemState() {}
    virtual std::string name() const = 0;
    virtual void makeAvailable(WorkItem&) {}   // TODO
    virtual void assign(WorkItem&)        {}   // TODO
    virtual void start(WorkItem&)         {}   // TODO
    virtual void complete(WorkItem&)      {}   // TODO
    virtual void reject(WorkItem&)        {}   // TODO
    virtual void cancel(WorkItem&)        {}   // TODO
    virtual void escalate(WorkItem&)      {}   // TODO
    virtual void suspend(WorkItem&)       {}   // TODO
    virtual void resume(WorkItem&)        {}   // TODO
};
