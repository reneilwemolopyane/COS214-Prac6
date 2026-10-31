/**
 * @file WorkflowEngine.cpp
 * @brief Implementation of WorkflowEngine.
 */
#include "tividy/command/WorkflowEngine.h"
#include "tividy/command/Command.h"
#include "tividy/command/CommandProcessor.h"
#include "tividy/core/Participant.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/observer/Observers.h"
#include "tividy/adapter/ExternalConnector.h"
#include <cstdlib>
#include <exception>
#include <sstream>

WorkflowEngine::~WorkflowEngine() {
    // Instances first: their work items release their assignee when destroyed.
    for (std::size_t i = 0; i < instances.size(); ++i)
        delete instances[i];
    for (std::size_t i = 0; i < participants.size(); ++i)
        delete participants[i];
}

bool WorkflowEngine::addParticipant(Participant* participant) {
    if (!participant) return false;
    participants.push_back(participant);
    return true;
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
    for (std::size_t i = 0; i < participants.size(); ++i)
        instance->addParticipant(participants[i]);
    instance->setClock(clockSeconds);
    instance->getNotificationService()->setConnector(connectorRegistry.find("notify"));   // optional SMS/email adapter
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
    instance->cancelAll();   // compensation reopens finished items, so close everything that is still open
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

int WorkflowEngine::advanceClock(long seconds) {
    if (seconds <= 0) return 0;
    clockSeconds += seconds;
    int moved = 0;
    for (std::size_t i = 0; i < instances.size(); ++i) {
        WorkflowInstance* in = instances[i];
        if (!isRunning(in)) continue;
        in->setClock(clockSeconds);
        moved += in->getCoordinator()->checkDeadlines();
    }
    return moved;
}

std::string WorkflowEngine::getOutcome(const std::string& instanceId) const {
    WorkflowInstance* instance = findInstance(instanceId);
    if (!instance) return "NOT_FOUND";
    return instance->getOutcome(getStatus(instanceId) == "CANCELLED");
}

void WorkflowEngine::refuse(WorkflowInstance* instance, const std::string& itemId, const std::string& why) {
    lastError = why;
    if (instance) instance->raiseEvent(EVT_REFUSED, itemId, 0, why);   // invalid actions stay visible in the history
}

bool WorkflowEngine::actorMayAct(WorkflowInstance* instance, WorkItem* item, const char* what) {
    Participant* actor = instance->getActor();
    if (actor && item->getAssignee() && item->getAssignee() != actor) {
        refuse(instance, item->getId(), std::string(what) + " refused: " + actor->getName() +
               " is not the assignee (" + item->getAssignee()->getName() + ")");
        return false;
    }
    return true;
}

bool WorkflowEngine::startWorkItem(WorkflowInstance* instance, WorkItem* item) {
    if (!item) return false;
    lastError.clear();
    if (isRunning(instance) && !actorMayAct(instance, item, "start")) return false;
    return runCommand(instance, new StartWorkItemCommand(item, "user", instance ? instance->frontOf(item->getId()) : 0));
}

bool WorkflowEngine::rejectWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& reason) {
    if (!item) return false;
    lastError.clear();
    if (isRunning(instance) && !actorMayAct(instance, item, "reject")) return false;
    return runCommand(instance, new RejectWorkItemCommand(item, reason.empty() ? "Rejected" : reason, "user"));
}

bool WorkflowEngine::escalateWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& reason) {
    if (!item || !isRunning(instance)) { lastError = "instance is not running"; return false; }
    lastError.clear();
    return runCommand(instance, new EscalateWorkItemCommand(instance->getCoordinator(), item->getId(), reason, "timer"));
}

bool WorkflowEngine::assignWorkItem(WorkflowInstance* instance, WorkItem* item, Participant* who) {
    if (!item) return false;
    lastError.clear();
    return runCommand(instance, new AssignWorkItemCommand(item, who));
}

// "$key" reads instance data; "$key*100" multiplies it (e.g. rand -> cents); anything else is a literal.
ConnectorRequest WorkflowEngine::buildServiceRequest(WorkflowInstance* instance, const ItemConfig& cfg) const {
    ConnectorRequest request;
    request.operation = cfg.serviceOperation;
    std::size_t start = 0;
    const std::string& spec = cfg.serviceParams;
    while (start < spec.size()) {
        std::size_t end = spec.find(';', start);
        std::string pair = spec.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq), value = pair.substr(eq + 1);
            if (!value.empty() && value[0] == '$') {
                std::size_t mul = value.find('*');
                std::string dataKey = value.substr(1, mul == std::string::npos ? std::string::npos : mul - 1);
                double factor = (mul == std::string::npos) ? 1.0 : std::atof(value.substr(mul + 1).c_str());
                value = instance->getData(dataKey);
                if (mul != std::string::npos) {
                    std::ostringstream out; out << static_cast<long>(std::atof(value.c_str()) * factor + 0.5);
                    value = out.str();
                }
            }
            request.params[key] = value;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return request;
}

bool WorkflowEngine::callItemService(WorkflowInstance* instance, WorkItem* item) {
    if (!item || !isRunning(instance)) { lastError = "instance is not running"; return false; }
    ItemConfig cfg = instance->getItemConfig(item->getId());
    if (cfg.serviceConnector.empty()) return true;
    ExternalConnector* connector = connectorRegistry.find(cfg.serviceConnector);
    if (!connector) { refuse(instance, item->getId(), "no connector registered as '" + cfg.serviceConnector + "'"); return false; }
    if (!invokeService(instance, connector, buildServiceRequest(instance, cfg), item->getId(),
                       cfg.serviceUndo.empty() ? std::string("reverse") : cfg.serviceUndo)) {
        if (lastError.empty()) lastError = "external system call failed (see incidents)";
        return false;
    }
    return true;
}

bool WorkflowEngine::completeWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& outcome) {
    if (!item) return false;
    lastError.clear();
    if (!isRunning(instance)) { lastError = "instance is not running"; return false; }
    if (!actorMayAct(instance, item, "complete")) return false;

    // Validate the transition BEFORE any side effect: a refused completion must never charge a payment.
    if (!item->isInProgress()) {
        refuse(instance, item->getId(), "Cannot complete work item '" + item->getName() + "' in state " + item->getStateName());
        return false;
    }
    // An item bound to an external system calls it first; the item only completes if the call succeeds.
    if (!callItemService(instance, item)) return false;
    return runCommand(instance, new CompleteWorkItemCommand(item, outcome, "user", instance->frontOf(item->getId())));
}

bool WorkflowEngine::invokeService(WorkflowInstance* instance, ExternalConnector* connector,
                                   const ConnectorRequest& request, const std::string& workItemId,
                                   const std::string& undoOperation) {
    if (!connector) return false;
    InvokeServiceCommand* command = new InvokeServiceCommand(connector, request, workItemId, undoOperation);
    bool ok = runCommand(instance, command);
    if (ok) {   // keep the system's answer with the instance (e.g. "Verify registration.valid")
        const ParamMap& answer = command->getResultData();
        for (ParamMap::const_iterator it = answer.begin(); it != answer.end(); ++it)
            instance->setData(workItemId + "." + it->first, it->second);
    } else if (lastError.empty()) {
        lastError = command->getError();
    }
    return ok;
}

bool WorkflowEngine::runCommand(WorkflowInstance* instance, WorkflowCommand* command) {
    // Only running instances accept work; otherwise nobody would own the command, so delete it.
    if (!isRunning(instance)) {
        delete command;
        lastError = "instance is not running";
        return false;
    }
    try {
        bool ok = instance->getProcessor()->run(command);
        if (!ok) lastError = command->getError();
        return ok;
    } catch (const std::exception& e) {
        // An invalid transition or a decorator refusing the action: record it, do not crash.
        refuse(instance, command->getWorkItemId(), e.what());
        return false;
    }
}

bool WorkflowEngine::isRunning(WorkflowInstance* instance) const {
    return instance && findInstance(instance->getId()) == instance && getStatus(instance->getId()) == "RUNNING";
}
