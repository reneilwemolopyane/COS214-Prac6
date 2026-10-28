#pragma once
#include <map>
#include <string>
#include <vector>
#include "WorkflowMediator.h"
#include "../strategy/AssignmentStrategy.h"

/**
 * @brief Strategy context, Mediator colleague and Prototype client
 * Owns clones of the strategies it is given
 */

 class WorkAssigner : public WorkflowColleague{
    private:
       std::map<std::string, AssignmentStrategy*> strategies;
       AssignmentStrategy* defaultStrategy;   
    public:
       WorkAssigner(WorkflowMediator* m = nullptr);
       ~WorkAssigner();
       WorkAssigner(const WorkAssigner&) = delete;
       WorkAssigner& operator=(const WorkAssigner&) = delete;

       /** @brief Used when an item's rule has no registered strategy. */
       void setDefaultStrategy(const AssignmentStrategy& prototype);

       /** @brief Items whose getAssignmentRule() == rule use a clone of prototype. */
       void registerStrategy(const std::string& rule, const AssignmentStrategy& prototype);

       /** @brief Picks a participant; notifies the mediator (FAILED) if none. */
       Participant* assign(WorkItem& item, const std::vector<Participant*>& pool);
 };