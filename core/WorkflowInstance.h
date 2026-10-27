#pragma once
#include <string>
#include "observer/ObserverPattern.h"
#include "visitor/Visitor.h"
#include "composite_prototype/WorkflowComponent.h"
#include "mediator/InstanceCoordinator.h"
#include "command/CommandProcessor.h"
#include "memento/Memento.h"

class WorkflowInstance : public WorkflowSubject, public WorkflowElement {
public:
    WorkflowInstance(const std::string& id, WorkflowComponent* rootCopy, const DataMap& d);
    ~WorkflowInstance();

    void start();
    void raiseEvent(EventType type, const std::string& workItemId = "",
                    WorkflowCommand* cause = 0, const std::string& message = "");
    void accept(WorkflowVisitor&);                      
    InstanceMemento* createMemento() const;  
    void restoreFromMemento(const InstanceMemento&); 
    std::string getId() const { return id; }
    CommandProcessor* getProcessor() { return processor; }
    InstanceCoordinator* getCoordinator() { return coordinator; }
private:
    std::string id; 
    WorkflowComponent* root; 
    DataMap data; 
    InstanceCoordinator* coordinator; 
    CommandProcessor* processor;
};