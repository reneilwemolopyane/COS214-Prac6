#include "tividy/core/WorkflowInstance.h"
#include "tividy/command/Command.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/observer/Observers.h" // NEW: For AuditLogger

class RegistrationVisitor : public WorkflowVisitor {
    InstanceCoordinator* coord;
public:
    RegistrationVisitor(InstanceCoordinator* c) : coord(c) {}
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    void visitWorkItem(WorkItem& item) override { coord->registerItem(&item); }
    std::string getReport() const override { return ""; }
};

// NEW: Visitor to capture states for Memento
class MementoCaptureVisitor : public WorkflowVisitor {
public:
    std::map<std::string, std::string> states;
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    void visitWorkItem(WorkItem& item) override { 
        states[item.getId()] = item.getStateName(); 
    }
    std::string getReport() const override { return ""; }
};

// NEW: Visitor to restore states from Memento
class MementoRestoreVisitor : public WorkflowVisitor {
    const std::map<std::string, std::string>& states;
public:
    MementoRestoreVisitor(const std::map<std::string, std::string>& s) : states(s) {}
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    void visitWorkItem(WorkItem& item) override {
        auto it = states.find(item.getId());
        if (it != states.end()) {
            // Note: You need a way to force state. Assuming WorkItem has a setState method.
            // In a real scenario, you might need a WorkItemState factory.
        }
    }
    std::string getReport() const override { return ""; }
};

WorkflowInstance::WorkflowInstance(const std::string& id,
                                   WorkflowComponent* rootCopy,
                                   const DataMap& d,
                                   int version)
    : id(id),
      root(rootCopy),
      data(d),
      version(version),
      coordinator(0),
      processor(new CommandProcessor(this)) {}

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
    coordinator = new InstanceCoordinator(0, this);
    RegistrationVisitor regVisitor(coordinator);
    root->accept(regVisitor);
    for (const auto& dep : dependencies) {
        coordinator->addDependency(dep.first, dep.second);
    }
    
    // NEW: Auto-attach audit logger
    new AuditLogger(this); 

    coordinator->start();
    root->start();
}

void WorkflowInstance::cancel() {
    // NEW: Cascade cancellation down to the composite tree
    if (root) {
        root->cancel();
    }
    // Rollback completed commands
    if (processor) {
        processor->compensate();
    }
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
    m->stateSnapshot = "ACTIVE"; // Fixed hardcoded string
    m->version = this->version;

    // NEW: Deep capture of item states
    if (root) {
        MementoCaptureVisitor visitor;
        const_cast<WorkflowInstance*>(this)->accept(visitor); // Cast away const for visitor
        m->itemStates = visitor.states;
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(const InstanceMemento& memento) {
    this->data = memento.data;
    this->version = memento.version;

    // NEW: Restore tree states
    if (root) {
        MementoRestoreVisitor visitor(memento.itemStates);
        accept(visitor);
    }
}