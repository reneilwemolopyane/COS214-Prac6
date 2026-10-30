#pragma once

#include <set>
#include <string>
#include <vector>

#include "tividy/mediator/WorkflowMediator.h"

class WorkItem;
class Participant;
class WorkAssigner;
class WorkflowInstance;
class WorkflowSubject;

struct Dependency {
    std::string item;
    std::string prerequisite;
};

/**
 * @brief Coordinates work items within one workflow instance.
 *
 * Tracks dependencies, handles work-item events, releases eligible
 * items and publishes events to observers.
 * Does not own the objects it references.
 */
class InstanceCoordinator : public WorkflowMediator {
private:
    WorkItem* findItem(const std::string& id) const;

    void closeItem(const std::string& id,
                   const std::string& reason);

    void handleCompleted(const std::string& id);
    void handleRejection(const std::string& id);
    void handleEscalation(const std::string& id);

    void publish(const WorkflowEvent& event) const;

    std::vector<WorkItem*> items;
    std::vector<Dependency> dependencies;
    std::set<std::string> completedItems;
    std::set<std::string> closedItems;
    std::vector<Participant*> pool;

    WorkAssigner* assigner;
    WorkflowSubject* events;
    bool started;
    WorkflowInstance* owner;

public:
    InstanceCoordinator(WorkAssigner* assigner,  WorkflowSubject* events, WorkflowInstance* instance);

    ~InstanceCoordinator() override;

    InstanceCoordinator& operator=(const InstanceCoordinator&) = delete;

    void notify(WorkflowColleague* sender, const WorkflowEvent& event) override;

    bool registerItem(WorkItem* item);

    void addDependency(const std::string& prerequisite, const std::string& item);

    void addParticipant(Participant* participant);

    void start();

    bool dependenciesMet(const std::string& itemId) const;

    void releaseAvailableItems();

    std::vector<WorkItem*> getAvailableItems() const;

    bool isFinished() const;
    void checkLifecycle()
    bool allCompleted() const;
};