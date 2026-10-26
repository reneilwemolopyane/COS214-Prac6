#pragma once
#include <string>
class WorkItem;
class WorkItemState {
public:
    virtual ~WorkItemState() {}
    virtual std::string name() const = 0;
    virtual void makeAvailable(WorkItem&);
    virtual void assign(WorkItem&);
    virtual void start(WorkItem&);
    virtual void complete(WorkItem&);
    virtual void reject(WorkItem&);
    virtual void cancel(WorkItem&);
    virtual void escalate(WorkItem&);
    virtual void suspend(WorkItem&);
    virtual void resume(WorkItem&);
};