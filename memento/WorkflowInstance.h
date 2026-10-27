#ifndef WORKFLOWINSTANCE_H
#define WORKFLOWINSTANCE_H

#include <string>
#include <iostream>

#include "WorkflowMemento.h"
//originator

class WorkflowInstance{

    public:
    WorkflowInstance(std::string version_,WorkItemState* state_);
    std::string getversion();
    WorkItemState* getState();
    void setVersion(std::string d);
    void display();
    WorkflowMemento*createMemento();
    void restore(WorkflowMemento* mem);

    private:
    WorkItemState* state;
    std::string version;

};

#endif