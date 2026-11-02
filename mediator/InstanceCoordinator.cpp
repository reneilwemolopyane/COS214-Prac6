// InstanceCoordinator.cpp
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/core/Participant.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/States.h"
#include "tividy/observer/Observers.h"
#include "tividy/interpreter/Expression.h"
#include "tividy/chain_of_responsibility/ApprovalRouter.h"
#include <cstdlib>
#include <iostream>

namespace {
/** Lets guards read instance data and the outcomes/states of work items. */
class CoordinatorContext : public EvaluationContext {
public:
    CoordinatorContext(const InstanceCoordinator& c, const DataMap* d) : coord(c), data(d) {}
    std::string lookup(const std::string& key) const {
    if (key.compare(0, 8, "outcome:") == 0) {
        WorkItem* i = coord.findItem(key.substr(8));
        std::cerr << "[LOOKUP outcome] '" << key.substr(8) << "' -> '"
                  << (i ? i->getOutcome() : "<item not found>") << "'\n";
        return i ? i->getOutcome() : "";
    }
    if (key.compare(0, 6, "state:") == 0) { WorkItem* i = coord.findItem(key.substr(6)); return i ? i->getStateName() : ""; }
    if (!data) { std::cerr << "[LOOKUP] data=NULL key='" << key << "'\n"; return ""; }
    DataMap::const_iterator it = data->find(key);
    std::string result = it == data->end() ? std::string() : it->second;
    std::cerr << "[LOOKUP] key='" << key << "' -> '" << result << "'\n";
    return result;
}
private:
    const InstanceCoordinator& coord; const DataMap* data;
};
}

InstanceCoordinator::InstanceCoordinator(WorkAssigner* a, WorkflowSubject* s)  : now(0), data(0), assigner(a), events(s), started(false){
    if (assigner) assigner->setMediator(this);
}

InstanceCoordinator::~InstanceCoordinator() {
    for (std::map<std::string, Expression*>::iterator g = guards.begin(); g != guards.end(); ++g) delete g->second;
    for (std::map<std::string, ApprovalRouter*>::iterator r = routers.begin(); r != routers.end(); ++r) delete r->second;
}

void InstanceCoordinator::publish(const WorkflowEvent& e) const {
    if (events) events->notifyObservers(e);
}

void InstanceCoordinator::notify(WorkflowColleague*, const WorkflowEvent& e){
    publish(e);                                  // log first: audit order stays true
    switch (e.type) {
        case EVT_COMPLETED: handleCompleted(e.workItemId); break;
        case EVT_SKIPPED: skippedItems.insert(e.workItemId); break;   // satisfies its dependents; no cascade
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

void InstanceCoordinator::addOrdering(const std::string& before, const std::string& after) {
    Dependency d; d.item = after; d.prerequisite = before;
    orderings.push_back(d);
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
    // A prerequisite is satisfied when it completed OR its branch was skipped.
    for (size_t i = 0; i < dependencies.size(); i++)
        if (dependencies[i].item == itemId &&
            completedItems.find(dependencies[i].prerequisite) == completedItems.end() &&
            skippedItems.find(dependencies[i].prerequisite) == skippedItems.end())
            return false;
    for (size_t i = 0; i < orderings.size(); i++)
        if (orderings[i].item == itemId &&
            completedItems.find(orderings[i].prerequisite) == completedItems.end() &&
            skippedItems.find(orderings[i].prerequisite) == skippedItems.end())
            return false;
    return true;
}

// Dead-path elimination: every prerequisite was skipped, so this item can never be reached.
bool InstanceCoordinator::onlySkippedPrerequisites(const std::string& itemId) const {
    bool any = false;
    for (size_t i = 0; i < dependencies.size(); i++) {
        if (dependencies[i].item != itemId) continue;
        any = true;
        if (skippedItems.find(dependencies[i].prerequisite) == skippedItems.end()) return false;
    }
    return any;
}

void InstanceCoordinator::setGuard(const std::string& itemId, const std::string& rule) {
    Expression* e = ExpressionParser::parse(rule);   // throws on bad syntax
    std::map<std::string, Expression*>::iterator old = guards.find(itemId);
    if (old != guards.end()) { delete old->second; old->second = e; } else guards[itemId] = e;
}

void InstanceCoordinator::setApproval(const std::string& itemId, const std::string& amountKey, const std::string& levels) {
    ApprovalRouter* r = ApprovalRouter::parse(levels);   // throws on bad text
    std::map<std::string, ApprovalRouter*>::iterator old = routers.find(itemId);
    if (old != routers.end()) { delete old->second; old->second = r; } else routers[itemId] = r;
    amountKeys[itemId] = amountKey;
}

double InstanceCoordinator::amountFor(const std::string& itemId) const {
    std::map<std::string, std::string>::const_iterator k = amountKeys.find(itemId);
    if (k == amountKeys.end() || !data) return 0;
    DataMap::const_iterator v = data->find(k->second);
    return v == data->end() ? 0 : std::atof(v->second.c_str());
}

std::string InstanceCoordinator::approvalLevel(const std::string& itemId) const {
    WorkItem* it = findItem(itemId);
    return it ? it->getData("approval.level") : std::string();
}

void InstanceCoordinator::armDeadline(const std::string& itemId) {
    std::map<std::string, long>::const_iterator d = deadlines.find(itemId);
    if (d != deadlines.end()) dueAt[itemId] = now + d->second;
}

int InstanceCoordinator::checkDeadlines() {
    int moved = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        WorkItem* it = items[i];
        std::map<std::string, long>::iterator due = dueAt.find(it->getId());
        if (due == dueAt.end() || due->second > now) continue;
        if (!it->isWaitingForPickup()) { dueAt.erase(due); continue; }   // picked up in time
        if (!canEscalate(it->getId())) {
            publish(WorkflowEvent(EVT_ROUTED, nullptr, it->getId(), nullptr, "deadline passed; nobody configured to take over"));
            dueAt.erase(due);
            continue;
        }
        const std::string reason = "deadline passed (no response)";
        if (escalate(it->getId(), reason)) ++moved;      // re-arms the deadline for the next level
        else { std::map<std::string, long>::iterator again = dueAt.find(it->getId()); if (again != dueAt.end()) dueAt.erase(again); }
    }
    return moved;
}

void InstanceCoordinator::skipItem(WorkItem* item, const std::string& reason) {
    item->skip(reason);   // raises SKIPPED through notify(), which records it
}

void InstanceCoordinator::routeApproval(WorkItem& item) {
    std::map<std::string, ApprovalRouter*>::iterator r = routers.find(item.getId());
    if (r == routers.end()) return;
    double amount = amountFor(item.getId());
    std::string note;
    int level = r->second->route(amount, -1, note);
    bool overflow = (level < 0);
    if (overflow) level = static_cast<int>(r->second->levelCount()) - 1;   // above every limit: top level decides
    const std::string& role = r->second->levelName(static_cast<size_t>(level));
    item.setRole(role);
    item.setData("approval.level", role);
    std::string msg = "approval level " + role + " (amount " + (data && data->count(amountKeys[item.getId()]) ? data->find(amountKeys[item.getId()])->second : std::string("0")) + ")";
    if (overflow) msg += ", above every limit";
    publish(WorkflowEvent(EVT_ROUTED, nullptr, item.getId(), nullptr, msg));
}

bool InstanceCoordinator::escalate(const std::string& itemId, const std::string& reason) {
    WorkItem* it = findItem(itemId);
    if (!it) return false;
    std::string role, from = it->getRole();
    std::map<std::string, ApprovalRouter*>::iterator r = routers.find(itemId);
    if (r != routers.end()) {
        int cur = r->second->indexOf(it->getData("approval.level"));
        std::string note;
        int next = r->second->route(amountFor(itemId), cur, note);
        if (next < 0 || next == cur) {
            publish(WorkflowEvent(EVT_ROUTED, nullptr, itemId, nullptr, "no higher approval level than " + from + "; request stays with them"));
            return false;
        }
        role = r->second->levelName(static_cast<size_t>(next));
    } else if (escalationRoles.count(itemId)) {
        role = escalationRoles[itemId];
    } else {
        it->escalate(reason);   // no one to hand it to: raise ESCALATED and close the item
        return true;
    }
    it->setRole(role);
    Participant* p = assigner ? assigner->assign(*it, pool) : nullptr;
    if (!p) { it->setRole(from); return false; }
    try { it->escalateTo(p); }
    catch (...) { it->setRole(from); throw; }
    if (r != routers.end()) it->setData("approval.level", role);
    armDeadline(itemId);
    publish(WorkflowEvent(EVT_ROUTED, nullptr, itemId, nullptr, "escalated " + from + " -> " + role + " (" + p->getName() + "): " + reason));
    return true;
}

bool InstanceCoordinator::isFinished()const{
    if (items.empty()) return false;
    // Skipped items are finished too (their branch was not taken).
    // An item can be in both sets (completed, then reopened by compensation and cancelled),
    // so test each item instead of adding the set sizes.
    for (size_t i = 0; i < items.size(); i++) {
        const std::string& id = items[i]->getId();
        if (completedItems.find(id) == completedItems.end() && closedItems.find(id) == closedItems.end() &&
            skippedItems.find(id) == skippedItems.end())
            return false;
    }
    return true;
}

void InstanceCoordinator::releaseAvailableItems() {
  bool changed = true;
  while (changed) {   // skipping an item can satisfy others, so repeat until nothing changes
    changed = false;
    for (size_t i = 0; i < items.size(); i++) {
        WorkItem* it = items[i];
        if (!it->isPending() || !dependenciesMet(it->getId())) continue;

        if (onlySkippedPrerequisites(it->getId())) {
            skipItem(it, "dead path: every prerequisite was skipped");
            changed = true;
            continue;
        }
        std::map<std::string, Expression*>::const_iterator g = guards.find(it->getId());
        if (g != guards.end()) {
            CoordinatorContext ctx(*this, data);
            if (!g->second->evaluate(ctx)) {
                skipItem(it, "branch not taken: " + g->second->toString());
                changed = true;
                continue;
            }
            publish(WorkflowEvent(EVT_ROUTED, nullptr, it->getId(), nullptr, "branch taken: " + g->second->toString()));
        }

        it->makeAvailable();
        routeApproval(*it);
        Participant* p = assigner ? assigner->assign(*it, pool) : nullptr;
        if (p) {                                  // null => assigner already notified FAILED
            it->assign(p);
            armDeadline(it->getId());
            publish(WorkflowEvent(EVT_ASSIGNED, nullptr, it->getId(), nullptr, "assignee: " + p->getName()));
        }
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
    if (items.empty() || completedItems.empty()) return false;
    for (size_t i = 0; i < items.size(); i++) {
        const std::string& id = items[i]->getId();
        if (completedItems.find(id) == completedItems.end() && skippedItems.find(id) == skippedItems.end()) return false;
    }
    return true;
}

WorkItem* InstanceCoordinator::findItem(const std::string& id)const{
    for (size_t i = 0; i < items.size(); i++){
        if (items[i]->getId() == id) return items[i];
    }
    return nullptr;
}

void InstanceCoordinator::handleCompleted(const std::string& id){
    completedItems.insert(id);

    WorkItem* it = findItem(id);
    if (it && data) {
        std::string safe;
        const std::string& raw = it->getId();
        for (std::size_t i = 0; i < raw.size(); ++i) {
            char c = raw[i];
            if (c == ' ')                                                      safe += '_';
            else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                     || (c >= '0' && c <= '9') || c == '_' || c == '-')        safe += c;
        }

        std::string outcome = it->getOutcome();
        std::string normalised;
        for (std::size_t k = 0; k < outcome.size(); ++k) {
            char ch = outcome[k];
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
            if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 32);
            normalised += ch;
        }

        (*data)[safe + "_outcome"] = normalised;
    }

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
   std::vector<Dependency> all(dependencies);
   all.insert(all.end(), orderings.begin(), orderings.end());   // a closed item blocks everything after it
   for (size_t i = 0; i < all.size(); i++) {
        if (all[i].prerequisite != id) continue;
        WorkItem* dep = findItem(all[i].item);
        if (!dep || !dep->isPending()) continue;
        dep->markEscalated();
        publish(WorkflowEvent(EVT_ESCALATED, nullptr, dep->getId(), nullptr, reason));
        closeItem(dep->getId(), "prerequisite " + dep->getId() + " was escalated");
    }
}
// Rebuild the mediator's derived indexes from restored work-item states.
// This intentionally emits no events and does not release newly available items.
void InstanceCoordinator::synchronizeAfterRestore() {
    completedItems.clear();
    closedItems.clear();
    skippedItems.clear();
    for (size_t i = 0; i < items.size(); ++i) {
        WorkItem* item = items[i];
        switch (item->getDisposition()) {   // the state object says what it means; no name comparisons
            case WorkItemState::DONE:       completedItems.insert(item->getId()); break;
            case WorkItemState::NOT_NEEDED: skippedItems.insert(item->getId()); break;
            case WorkItemState::BLOCKED:    closedItems.insert(item->getId()); break;
            case WorkItemState::OPEN:       break;
        }
    }
}
