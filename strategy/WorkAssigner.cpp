#include "WorkAssigner.h"
#include "WorkItem.h"
#include "WorkflowEvent.h"

WorkAssigner::WorkAssigner(WorkflowMediator* m) : defaultStrategy(nullptr){
    setMediator(m);
}

WorkAssigner::~WorkAssigner{
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
        DataMap d;
        d["reason"]   = "no eligible participant";
        d["strategy"] = chosen ? chosen->name() : "none";
        mediator->notify(this, WorkflowEvent(EVT_FAILED, item.getId(), d));
    }
    return p;
}