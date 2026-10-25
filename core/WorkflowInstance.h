#pragma once
#include <string>
#include "tividy/observer/ObserverPattern.h"
#include "tividy/visitor/Visitor.h"
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/command/CommandProcessor.h"
#include "tividy/memento/Memento.h"
/** @brief ConcreteSubject, Visitor element, Memento originator, Composite client. STUB */
class WorkflowInstance : public WorkflowSubject, public WorkflowElement {
public:
    WorkflowInstance(const std::string& id, WorkflowComponent* rootCopy, const DataMap& d)
        : id(id), root(rootCopy), data(d), coordinator(0), processor(0) {}
    void start() {}                                       // TODO
    void accept(WorkflowVisitor&) {}                      // TODO
    InstanceMemento* createMemento() const { return 0; }  // TODO
    void restoreFromMemento(const InstanceMemento&) {}    // TODO
    std::string getId() const { return id; }
    CommandProcessor* getProcessor() { return processor; }
    InstanceCoordinator* getCoordinator() { return coordinator; }
private:
    std::string id; WorkflowComponent* root; DataMap data;
    InstanceCoordinator* coordinator; CommandProcessor* processor;
};
