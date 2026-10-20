#pragma once
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/Mediator.h"
#include "tividy/state/WorkItemState.h"
/** @brief Leaf (Composite), ConcretePrototype, State context, ConcreteColleague. STUB */
class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    explicit WorkItem(const std::string& name = "") : name(name), state(0) {}
    void setState(WorkItemState*) {}              // TODO
    const WorkItemState* getState() const { return state; }
    void makeAvailable() {}  void assign() {}  void start() {}  void complete() {}
    void reject() {}  void cancel() {}  void escalate() {}  void suspend() {}  void resume() {}   // TODO: delegate to state
    bool isComplete() const { return false; }     // TODO
    WorkflowComponent* clone() const { return 0; }  // TODO: copy with fresh state
    std::string getName() const { return name; }
    void accept(WorkflowVisitor&) {}              // TODO
private:
    std::string name;
    WorkItemState* state;
};
