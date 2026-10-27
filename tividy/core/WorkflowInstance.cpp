#include "tividy/core/WorkflowInstance.h"
#include "tividy/command/Command.h"

class RegistrationVisitor : public WorkflowVisitor {
    InstanceCoordinator* coord;
public:
    RegistrationVisitor(InstanceCoordinator* c) : coord(c) {}

    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}

    void visitWorkItem(WorkItem& item) override {
        coord->registerItem(&item);
    }

    std::string getReport() const override { return ""; }
};

WorkflowInstance::WorkflowInstance(const std::string& id, WorkflowComponent* rootCopy, const DataMap& d)
    : id(id), root(rootCopy), data(d), coordinator(0), processor(new CommandProcessor(this))
{
}

WorkflowInstance::~WorkflowInstance() {
    delete root;
    delete coordinator;
    delete processor;
}

void WorkflowInstance::start() {
    if (!root) return;
    coordinator = new InstanceCoordinator(0, this);
    RegistrationVisitor regVisitor(coordinator);
    root->accept(regVisitor);
    coordinator->start();
    root->start();
}

void WorkflowInstance::raiseEvent(EventType type, const std::string& workItemId,
                                  WorkflowCommand* cause, const std::string& message) {
    DataMap payload;
    payload["instance"] = id;
    if (!message.empty()) payload["message"] = message;
    if (cause) payload["command"] = cause->name();
    notifyObservers(WorkflowEvent(type, workItemId, payload, cause, this));
}

void WorkflowInstance::accept(WorkflowVisitor& v) {
    v.visitInstance(*this);
    if (root) {
        root->accept(v);
    }
}

InstanceMemento* WorkflowInstance::createMemento() const {
    InstanceMemento* m = new InstanceMemento();
    m->data = this->data;
    m->stateSnapshot = "SNAPSHOT";
    return m;
}

void WorkflowInstance::restoreFromMemento(const InstanceMemento& memento) {
    this->data = memento.data;
}