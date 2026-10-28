#pragma once
#include <string>
#include "tividy/observer/ObserverPattern.h"
#include "tividy/visitor/Visitor.h"
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/command/CommandProcessor.h"
#include "tividy/memento/Memento.h"

class WorkflowInstance : public WorkflowSubject, public WorkflowElement {
public:
    WorkflowInstance(const std::string& id,
                     WorkflowComponent* rootCopy,
                     const DataMap& d,
                     int version);

    ~WorkflowInstance();

    void start();
    void accept(WorkflowVisitor&);
    InstanceMemento* createMemento() const;
    void restoreFromMemento(const InstanceMemento&);

    std::string getId() const { return id; }
    int getVersion() const { return version; }

    CommandProcessor* getProcessor() { return processor; }
    InstanceCoordinator* getCoordinator() { return coordinator; }

private:
    std::string id;
    WorkflowComponent* root;
    DataMap data;
    int version;
    InstanceCoordinator* coordinator;
    CommandProcessor* processor;
};