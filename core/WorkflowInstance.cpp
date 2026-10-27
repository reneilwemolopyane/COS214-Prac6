#include "tividy/core/WorkflowInstance.h"


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
    : id(id), root(rootCopy), data(d), coordinator(0), processor(0) 
{
}

WorkflowInstance::~WorkflowInstance() {
    
    delete root;
    delete coordinator;
    delete processor;
}

void WorkflowInstance::start() {
    if (!root) return;
    processor = new CommandProcessor(this);
    coordinator = new InstanceCoordinator(0, this); 
    RegistrationVisitor regVisitor(coordinator);
    root->accept(regVisitor);
    coordinator->start();
    root->start();
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