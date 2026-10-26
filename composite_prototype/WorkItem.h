#pragma once
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/Mediator.h"
#include "tividy/state/WorkItemState.h"

class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    explicit WorkItem(const std::string& name = "");
    void setState(WorkItemState*);
    const WorkItemState* getState() const;
    void makeAvailable();
    void assign();
    void start();
    void complete();
    void reject();
    void cancel();
    void escalate();
    void suspend();
    void resume();
    bool isComplete() const;
    WorkflowComponent* clone() const;
    std::string getName() const;
    void accept(WorkflowVisitor&);
private:
    std::string name;
    WorkItemState* state;
};