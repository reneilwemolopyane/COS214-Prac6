#include "tividy/strategy/WorkAssigner.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Participant.h"

WorkAssigner::WorkAssigner(WorkflowMediator* m) : defaultStrategy(nullptr){
    setMediator(m);
}

WorkAssigner::~WorkAssigner(){
    delete defaultStrategy;
    for(std::map<std::string, AssignmentStrategy*>::iterator it = strategies.begin(); it != strategies.end(); ++it){
        delete it->second;
    }
}

void WorkAssigner::setDefaultStrategy(const AssignmentStrategy& prototype){
    delete defaultStrategy;
    defaultStrategy = prototype.clone();
}

void WorkAssigner::registerStrategy(const std::string& rule, const AssignmentStrategy& prototype){
    std::map<std::string, AssignmentStrategy*>::iterator it = strategies.find(rule);
    if (it != strategies.end()) {            // replacing a rule: avoid leaking the old one
        delete it->second;
        it->second = prototype.clone();
    } else {
        strategies[rule] = prototype.clone();
    }
}

Participant* WorkAssigner::assign(WorkItem& item, const std::vector<Participant*>& pool){
    AssignmentStrategy* chosen = defaultStrategy;
    std::map<std::string, AssignmentStrategy*>::iterator it =
        strategies.find(item.getAssignmentRule());
    if (it != strategies.end()) chosen = it->second;

    Participant* p = chosen ? chosen->selectParticipant(item, pool) : nullptr;

    if (!p && mediator) {
        std::string strategyName = chosen ? chosen->name() : std::string("none");
        mediator->notify(this, WorkflowEvent(EVT_FAILED, nullptr, item.getId(), nullptr, "no eligible participant (strategy: " + strategyName + ")"));
    }
    return p;
}