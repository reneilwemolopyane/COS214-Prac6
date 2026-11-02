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
    /** @brief Branch not taken: Created -> Skipped (dead-path elimination). */
    virtual void skip(WorkItem& i);

    /** How the mediator sees a state: still open, done, branch not needed, or closed without success. */
    enum Disposition { OPEN, DONE, NOT_NEEDED, BLOCKED };
    virtual Disposition disposition() const { return OPEN; }
    virtual bool isRejection() const { return false; }
    /** @return the shared state object with this name (used when a memento restores an item), or null. */
    static WorkItemState* byName(const std::string& stateName);

    virtual bool isFinal() const { return false; }
    virtual bool isCreated() const { return false; }
    virtual bool isAvailable() const { return false; }
};