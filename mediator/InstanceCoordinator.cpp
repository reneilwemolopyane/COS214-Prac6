// InstanceCoordinator.cpp
#include "InstanceCoordinator.h"
#include "ConnectorRegistry.h"
#include "Participant.h"
#include "WorkAssigner.h"
#include "WorkItem.h"
#include "WorkflowSubject.h"

InstanceCoordinator::InstanceCoordinator(WorkAssigner* a, WorkflowSubject* s, ConnectorRegistry* c)
    : assigner(a), events(s), connectors(c) {
    if (assigner) assigner->setMediator(this);
}

InstanceCoordinator::~InstanceCoordinator() {}   // owns nothing

void InstanceCoordinator::publish(const WorkflowEvent& e) const {
    if (events) events->notifyObservers(e);
}

void InstanceCoordinator::notify(WorkflowColleague*, const WorkflowEvent& e){
    publish(e);                                  // log first: audit order stays true
    switch (e.getType()) {
        case EventType::COMPLETED: handleCompleted(e.getWorkItemId()); break;
        case EventType::REJECTED:  handleRejection(e.getWorkItemId()); break;
        case EventType::FAILED:
        case EventType::TIMED_OUT:
        case EventType::ESCALATED: handleEscalation(e.getWorkItemId()); break;
        default: break;   // CREATED, ASSIGNED, STARTED, COMPENSATED: announcements only
    }
}

bool InstanceCoordinator::registerItem(WorkItem* item) {
    if (!item || findItem(item->getId())) return false;
    items.push_back(item);
    item->setMediator(this);
    return true;
}

void InstanceCoordinator::addDependency(const std::string& item, const std::string& pre){
    Dependency d;
    d.item = item;
    d.prerequisite = pre;
    dependencies.push_back(d);
}

void InstanceCoordinator::addParticipant(Participant* p){
    if (p) pool.push_back(p);
}

void InstanceCoordinator::start() {
    publish(WorkflowEvent(EventType::CREATED, ""));
    releaseAvailableItems();
}

bool InstanceCoordinator::dependenciesMet(const std::string& itemId)const{
    for (size_t i = 0; i < dependencies.size(); i++)
        if (dependencies[i].item == itemId &&
            completedItems.find(dependencies[i].prerequisite) == completedItems.end())
            return false;
    return true;
}

void InstanceCoordinator::releaseAvailableItems() {
    for (size_t i = 0; i < items.size(); i++) {
        WorkItem* it = items[i];
        if (!it->isPending() || !dependenciesMet(it->getId())) continue;

        it->makeAvailable();
        Participant* p = assigner ? assigner->assign(*it, pool) : nullptr;
        if (p) {                                  // null => assigner already notified FAILED
            it->setAssignee(p);
            DataMap d;
            d["assignee"] = p->getName();
            publish(WorkflowEvent(EventType::ASSIGNED, it->getId(), d));
        }
    }
}

std::vector<WorkItem*> InstanceCoordinator::getAvailableItems()const{
    std::vector<WorkItem*> out;
    for (size_t i = 0; i < items.size(); i++)
        if (items[i]->isAvailable()) out.push_back(items[i]);
    return out;
}

bool InstanceCoordinator::allCompleted()const{
    return !items.empty() && completedItems.size() == items.size();
}

WorkItem* InstanceCoordinator::findItem(const std::string& id)const{
    for (size_t i = 0; i < items.size(); i++){
        if (items[i]->getId() == id) return items[i];
    }
    return nullptr;
}

void InstanceCoordinator::handleCompleted(const std::string& id){
    completedItems.insert(id);
    releaseAvailableItems();                      
}

void InstanceCoordinator::handleRejection(const std::string& id){
    WorkItem* it = findItem(id);
    if (it) it->markFailed();                     
}

void InstanceCoordinator::handleEscalation(const std::string& id){
    WorkItem* it = findItem(id);
    if (it) it->markEscalated();
}

bool InstanceCoordinator::callExternal(const std::string& key, const ConnectorRequest& req, ConnectorResult& out, int maxAttempts){
    if (!connectors) {
        out = ConnectorResult();
        out.message = "No connector registry configured";
    } else {
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            out = connectors->execute(key, req);
            if (out.success) return true;
            if (!out.retryable) break;            
        }
    }
    DataMap d;
    d["connector"] = key;
    d["message"]   = out.message;
    notify(nullptr, WorkflowEvent(EventType::ESCALATED, req.itemId, d));
    return false;
}