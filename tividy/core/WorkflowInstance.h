#pragma once
#include <string>
#include <vector>
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
                     int version = 1);

    ~WorkflowInstance();

    void start();
    void addDependency(const std::string& prerequisite, const std::string& item);
    void raiseEvent(EventType type, const std::string& workItemId = "",
                    WorkflowCommand* cause = 0, const std::string& message = "");
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
    std::vector<std::pair<std::string,std::string>> dependencies;
    InstanceCoordinator* coordinator;
    CommandProcessor* processor;
};