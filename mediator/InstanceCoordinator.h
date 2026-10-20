#pragma once
#include "tividy/mediator/Mediator.h"
/** @brief ConcreteMediator: one per WorkflowInstance. Holds dependency state. STUB */
class InstanceCoordinator : public WorkflowMediator {
public:
    void notify(WorkflowColleague*, const WorkflowEvent&) {}  // TODO: dependency checks, makeAvailable, assign
};
