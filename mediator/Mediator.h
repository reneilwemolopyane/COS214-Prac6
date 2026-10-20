#pragma once
/** @file Mediator.h  Mediator interfaces. STUB */
#include "tividy/core/WorkflowEvent.h"
class WorkflowColleague;

class WorkflowMediator {
public:
    virtual ~WorkflowMediator() {}
    virtual void notify(WorkflowColleague* sender, const WorkflowEvent& event) = 0;
};
class WorkflowColleague {
public:
    WorkflowColleague() : mediator(0) {}
    virtual ~WorkflowColleague() {}
    void setMediator(WorkflowMediator* m) { mediator = m; }
protected:
    WorkflowMediator* mediator;
};
