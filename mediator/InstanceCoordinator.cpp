// InstanceCoordinator.cpp
#include "InstanceCoordinator.h"
#include "../core/Participant.h"
#include "../strategy/WorkAssigner.h"
#include "../composite_state/WorkItem.h"
#include "../observer/Observers.h"

InstanceCoordinator::InstanceCoordinator(WorkAssigner* a, WorkflowSubject* s)  : assigner(a), events(s), started(false){
    if (assigner) assigner->setMediator(this);
}

InstanceCoordinator::~InstanceCoordinator() {}   // owns nothing

void InstanceCoordinator::publish(const WorkflowEvent& e) const {
    if (events) events->notifyObservers(e);
}

void InstanceCoordinator::notify(WorkflowColleague*, const WorkflowEvent& e){
    publish(e);                                  // log first: audit order stays true
    switch (e.Type) {
        case EVT_COMPLETED: handleCompleted(e.workItemId); break;
        case EVT_REJECTED:
        case EVT_CANCELLED: handleRejection(e.workItemId); break;
        case EVT_FAILED:
        case EVT_TIMED_OUT:
        case EVT_ESCALATED: handleEscalation(e.workItemId); break;
        default: break;   // CREATED, ASSIGNED, STARTED, COMPENSATED: announcements only
    }
}

bool InstanceCoordinator::registerItem(WorkItem* item) {
    if (!item || findItem(item->getId())) return false;
    items.push_back(item);
    item->setMediator(this);
    return true;
}

void InstanceCoordinator::addDependency(const std::string& prerequisite, const std::string& item){
    Dependency d;
    d.item = item;
    d.prerequisite = prerequisite;
    dependencies.push_back(d);
}

void InstanceCoordinator::addParticipant(Participant* p){
    if (p) pool.push_back(p);
}

void InstanceCoordinator::start() {
    if(started) return;
    started = true;
    publish(WorkflowEvent(EVT_CREATED, nullptr));
    releaseAvailableItems();
}

bool InstanceCoordinator::dependenciesMet(const std::string& itemId)const{
    for (size_t i = 0; i < dependencies.size(); i++)
        if (dependencies[i].item == itemId &&
            completedItems.find(dependencies[i].prerequisite) == completedItems.end())
            return false;
    return true;
}

bool InstanceCoordinator::isFinished()const{
    return !items.empty() && completedItems.size() + closedItems.size() == items.size();
}

void InstanceCoordinator::releaseAvailableItems() {
    for (size_t i = 0; i < items.size(); i++) {
        WorkItem* it = items[i];
        if (!it->isPending() || !dependenciesMet(it->getId())) continue;

        it->makeAvailable();
        Participant* p = assigner ? assigner->assign(*it, pool) : nullptr;
        if (p) {                                  // null => assigner already notified FAILED
            it->assign(p);
            publish(WorkflowEvent(EVT_ASSIGNED, nullptr it->getId(), nullptr,  "assignee: " + p->getName()));
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
   closeItem(id, "prerequisite " + id + " was closed") ;                 
}

void InstanceCoordinator::handleEscalation(const std::string& id){
    WorkItem* it = findItem(id);
    if (it && closedItems.find(id) == closedItems.end()){
        if (it) it->markEscalated();
    }
    closeItem(id, "prerequisite " + id + " was escalated");
    
}

void InstanceCoordinator::closeItem(const std::string& id, const std::string& reason) {
   if(!closedItems.insert(id).second) return;
   for (size_t i = 0; i < dependencies.size(); i++) {
        if (dependencies[i].prerequisite != id) continue;
        WorkItem* dep = findItem(dependencies[i].item);
        if (!dep || !dep->isPending()) continue;
        dep->markEscalated();
        publish(WorkflowEvent(EVT_ESCALATED, nullptr, dep->getId(), nullptr, reason));
        closeItem(dep->getId(), "prerequisite " + dep->getId() + " was escalated");
    }
}