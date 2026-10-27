#pragma once
#include <string>
#include "tividy/state/InvalidTransitionException.h"

class WorkItem;

class WorkItemState {
public:
    virtual ~WorkItemState() {}
    virtual std::string name() const = 0;

    virtual void makeAvailable(WorkItem& i);
    virtual void assign(WorkItem& i);
    virtual void start(WorkItem& i);
    virtual void complete(WorkItem& i);
    virtual void reject(WorkItem& i);
    virtual void cancel(WorkItem& i);
    virtual void escalate(WorkItem& i);
    virtual void suspend(WorkItem& i);
    virtual void resume(WorkItem& i);

    virtual bool isFinal() const { return false; }
    virtual bool isCreated() const { return false; }
    virtual bool isAvailable() const { return false; }
};