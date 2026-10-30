#include "tividy/core/WorkflowInstance.h"
#include "tividy/command/Command.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"


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

WorkflowInstance::WorkflowInstance(const std::string& id,
                                   WorkflowComponent* rootCopy,
                                   const DataMap& d,
                                   int version,
                                   WorkAssigner* assigner,
                                   const std::vector<Participant*>& pool)
    : id(id),
      root(rootCopy),
      data(d),
      assigner(assigner), 
      pool(pool),
      version(version),   
      coordinator(0),
      processor(new CommandProcessor(this)) 
{
}

WorkflowInstance::~WorkflowInstance() {
    delete root;
    delete coordinator;
    delete processor;
}

void WorkflowInstance::addDependency(const std::string& prerequisite, const std::string& item) {
    dependencies.push_back(std::make_pair(prerequisite, item));
}

void WorkflowInstance::start() {
    if (!root) return;
    delete coordinator;
    coordinator = new InstanceCoordinator(assigner, this);
    for (size_t i = 0; i < pool.size(); ++i) {
        coordinator->addParticipant(pool[i]);
    }
    RegistrationVisitor regVisitor(coordinator);
    root->accept(regVisitor);
    for (const auto& dep : dependencies) {
        coordinator->addDependency(dep.first,dep.second);
    }
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
    m->version = this->version;
    return m;
}

void WorkflowInstance::restoreFromMemento(const InstanceMemento& memento) {
    this->data = memento.data;
   this->version = memento.version;
}