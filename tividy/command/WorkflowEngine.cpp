/**
 * @file WorkflowEngine.cpp
 * @brief Implementation of WorkflowEngine.
 */
#include "tividy/command/WorkflowEngine.h"
#include "tividy/command/Command.h"
#include "tividy/command/CommandProcessor.h"

namespace {
// NEW: Helper Visitor for ownership validation
class InstanceItemValidator : public WorkflowVisitor {
public:
    WorkItem* target;
    bool found;
    InstanceItemValidator(WorkItem* t) : target(t), found(false) {}
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    void visitWorkItem(WorkItem& i) override { if (&i == target) found = true; }
    std::string getReport() const override { return ""; }
};
}

WorkflowEngine::~WorkflowEngine() {
    for (std::size_t i = 0; i < instances.size(); ++i)
        delete instances[i];
}

bool WorkflowEngine::registerInstance(WorkflowInstance* instance) {
    if (!instance || status.count(instance->getId())) return false;
    instances.push_back(instance);
    status[instance->getId()] = "REGISTERED";
    return true;
}

bool WorkflowEngine::start(WorkflowInstance* instance) {
    if (!instance || findInstance(instance->getId()) != instance) return false;
    std::string& s = status[instance->getId()];
    if (s != "REGISTERED") return false;   // Each instance starts once.
    s = "RUNNING";
    instance->start();
    instance->raiseEvent(EVT_STARTED);
    return true;
}

bool WorkflowEngine::start(const std::string& instanceId) {
    return start(findInstance(instanceId));
}

bool WorkflowEngine::cancel(const std::string& instanceId) {
    WorkflowInstance* instance = findInstance(instanceId);
    if (!isRunning(instance)) return false;
    // Reverse everything already done in this instance (e.g. refund a payment) before cancelling.
    instance->getProcessor()->compensate();
    status[instanceId] = "CANCELLED";
    instance->raiseEvent(EVT_CANCELLED);
    return true;
}

WorkflowInstance* WorkflowEngine::findInstance(const std::string& instanceId) const {
    for (std::size_t i = 0; i < instances.size(); ++i)
        if (instances[i]->getId() == instanceId) return instances[i];
    return 0;
}

std::string WorkflowEngine::getStatus(const std::string& instanceId) const {
    std::map<std::string, std::string>::const_iterator it = status.find(instanceId);
    return it == status.end() ? "NOT_FOUND" : it->second;
}

bool WorkflowEngine::assignWorkItem(WorkflowInstance* instance, WorkItem* item, Participant* who) {
    if (!item) return false;
    
    // NEW: Security Check
    InstanceItemValidator validator(item);
    instance->accept(validator);
    if (!validator.found) return false; 

    return runCommand(instance, new AssignWorkItemCommand(item, who));
}

bool WorkflowEngine::completeWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& outcome) {
    if (!item) return false;

    // NEW: Security Check
    InstanceItemValidator validator(item);
    instance->accept(validator);
    if (!validator.found) return false; 

    // NEW: Invoke approval chain if needed
    // (This is a placeholder - you need to check if the item is wrapped in an ApprovalDecorator)
    // if (dynamic_cast<ApprovalDecorator*>(item)) {
    //     // run approval logic here
    // }

    return runCommand(instance, new CompleteWorkItemCommand(item, outcome));
}


bool WorkflowEngine::invokeService(WorkflowInstance* instance, ExternalConnector* connector,
                                   const ConnectorRequest& request, const std::string& workItemId,
                                   const std::string& undoOperation) {
    if (!connector) return false;
    return runCommand(instance, new InvokeServiceCommand(connector, request, workItemId, undoOperation));
}

bool WorkflowEngine::runCommand(WorkflowInstance* instance, WorkflowCommand* command) {
    // Only running instances accept work; otherwise nobody would own the command, so delete it.
    if (!isRunning(instance)) {
        delete command;
        return false;
    }
    return instance->getProcessor()->run(command);
}

bool WorkflowEngine::isRunning(WorkflowInstance* instance) const {
    return instance && findInstance(instance->getId()) == instance && getStatus(instance->getId()) == "RUNNING";
}