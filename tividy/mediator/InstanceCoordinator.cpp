#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/core/Participant.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/observer/Observers.h"
#include "tividy/core/WorkflowInstance.h"

InstanceCoordinator::InstanceCoordinator(WorkAssigner* a, WorkflowSubject* s, WorkflowInstance* instance)
    : assigner(a), events(s), started(false), owner(instance) {
    if (assigner) assigner->setMediator(this);
}

InstanceCoordinator::~InstanceCoordinator() {}

void InstanceCoordinator::publish(const WorkflowEvent& event) const {
    if (events) events->notifyObservers(event);
}

void InstanceCoordinator::notify(WorkflowColleague* sender, const WorkflowEvent& event) {
    (void)sender;
    publish(event);

    switch (event.type) {
        case EVT_COMPLETED:
            handleCompleted(event.workItemId);
            break;

        case EVT_REJECTED:
        case EVT_CANCELLED:
            handleRejection(event.workItemId);
            break;

        case EVT_FAILED:
        case EVT_TIMED_OUT:
        case EVT_ESCALATED:
            handleEscalation(event.workItemId);
            break;

        default:
            break;
    }
}

bool InstanceCoordinator::registerItem(WorkItem* item) {
    if (!item || findItem(item->getId())) return false;

    items.push_back(item);
    item->setMediator(this);
    return true;
}

void InstanceCoordinator::addDependency(const std::string& prerequisite, const std::string& item) {
    if (item == prerequisite || !findItem(item) || !findItem(prerequisite)) return;

    for (size_t i = 0; i < dependencies.size(); i++) {
        if (dependencies[i].item == item &&
            dependencies[i].prerequisite == prerequisite) {
            return;
        }
    }

    dependencies.push_back({item, prerequisite});
}

void InstanceCoordinator::addParticipant(Participant* participant) {
    if (participant) pool.push_back(participant);
}

void InstanceCoordinator::start() {
    if (started) return;

    started = true;
    publish(WorkflowEvent(EVT_CREATED, nullptr));
    releaseAvailableItems();

    checkLifecycle();
}

bool InstanceCoordinator::dependenciesMet(const std::string& itemId) const {
    for (size_t i = 0; i < dependencies.size(); i++) {
        if (dependencies[i].item == itemId &&
            completedItems.find(dependencies[i].prerequisite) == completedItems.end()) {
            return false;
        }
    }

    return true;
}

bool InstanceCoordinator::isFinished() const {
    return !items.empty() && completedItems.size() + closedItems.size() == items.size();
}

bool InstanceCoordinator::allCompleted() const {
    return !items.empty() && completedItems.size() == items.size();
}

void InstanceCoordinator::releaseAvailableItems() {
    for (size_t i = 0; i < items.size(); i++) {
        WorkItem* item = items[i];

        if (!item->isPending() || !dependenciesMet(item->getId())) continue;

        item->makeAvailable();

        Participant* participant = assigner ? assigner->assign(*item, pool) : nullptr;

        if (participant) {
            item->assign(participant);

            publish(WorkflowEvent(
                EVT_ASSIGNED,
                nullptr,
                item->getId(),
                nullptr,
                "assignee: " + participant->getName()
            ));
        }
    }
}

std::vector<WorkItem*> InstanceCoordinator::getAvailableItems() const {
    std::vector<WorkItem*> result;

    for (size_t i = 0; i < items.size(); i++) {
        if (items[i]->isAvailable()) result.push_back(items[i]);
    }

    return result;
}

WorkItem* InstanceCoordinator::findItem(const std::string& id) const {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i]->getId() == id) return items[i];
    }

    return nullptr;
}

void InstanceCoordinator::handleCompleted(const std::string& id) {
    if (!findItem(id) || closedItems.count(id)) return;

    completedItems.insert(id);
    releaseAvailableItems();
    checkLifecycle();
}

void InstanceCoordinator::handleRejection(const std::string& id) {
    closeItem(id, "prerequisite " + id + " was rejected or cancelled");
}

void InstanceCoordinator::handleEscalation(const std::string& id) {
    WorkItem* item = findItem(id);

    if (!item || closedItems.count(id)) return;

    item->markEscalated();
    closeItem(id, "prerequisite " + id + " was escalated");
}

void InstanceCoordinator::checkLifecycle() {
    if (!owner || !isFinished()) return;

    if (allCompleted()) {
        owner->updateStatus(COMPLETED);
    }
    else {
        owner->updateStatus(TERMINATED);
    }
}

void InstanceCoordinator::closeItem(const std::string& id, const std::string& reason) {
    if (completedItems.count(id) || !closedItems.insert(id).second) return;

    for (size_t i = 0; i < dependencies.size(); i++) {
        if (dependencies[i].prerequisite != id) continue;

        WorkItem* dependent = findItem(dependencies[i].item);

        if (!dependent || !dependent->isPending() ||
            completedItems.count(dependent->getId()) ||
            closedItems.count(dependent->getId())) {
            continue;
        }

        dependent->markEscalated();

        publish(WorkflowEvent(
            EVT_ESCALATED,
            nullptr,
            dependent->getId(),
            nullptr,
            reason
        ));

        closeItem(dependent->getId(), "prerequisite " + dependent->getId() + " was escalated");
        checkLifecycle();
    }
}