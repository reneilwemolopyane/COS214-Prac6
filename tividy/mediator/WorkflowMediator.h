#pragma once

<<<<<<< HEAD:mediator/WorkflowMediator.h
#include "../core/WorkflowEvent.h"
=======
#include "tividy/core/WorkflowEvent.h"
>>>>>>> e4fd11442553d70b81b0be8c4c09ca7a6b130ef9:tividy/mediator/WorkflowMediator.h
class WorkflowColleague;

/** @brief Mediator interface: colleagues talk to this, never to each other */
class WorkflowMediator {
public:
    virtual ~WorkflowMediator();
    virtual void notify(WorkflowColleague* sender, const WorkflowEvent& event) = 0;
};

/** @brief Colleague base : holds a non-owning pointer to its mediator */
class WorkflowColleague {
public:
    WorkflowColleague() : mediator(0) {}
    virtual ~WorkflowColleague() {}
    void setMediator(WorkflowMediator* m) { mediator = m; }
protected:
    WorkflowMediator* mediator;
};
