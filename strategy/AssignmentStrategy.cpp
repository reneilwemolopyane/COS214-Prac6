#include "AssignmentStrategy.h"


AssignmentStrategy::~AssignmentStrategy(){}

//Role based Strategy
Participant* RoleBasedStrategy::selectParticipant(const WorkItem&, const std::vector<Participant*>& candidates){
    std::vector<Participant*> ok = eligible(item, candidates);
    return ok.empty() ? nullptr : ok[0];
AssignmentStrategy* RoleBasedStrategy::clone()const{
    return new RoleBasedStrategy(*this);
}
std::string RoleBasedStrategy::name()const{
    return "RoleBased";
}

//Shortest Queue Strategy
Participant* ShortestQueueStrategy::selectParticipant(const WorkItem&, const std::vector<Participant*>&){
    if (ok.empty()) return nullptr;
    Participant* best = ok[0];
    for (int i = 1; i < (int)ok.size(); i++)
        if (ok[i]->getWorkload() < best->getWorkload()) best = ok[i];
    return best;
} 
AssignmentStrategy* ShortestQueueStrategy::clone()const{
    return new ShortestQueueStrategy(*this);
}
std::string ShortestQueueStrategy::name()const{
    return "ShortestQueue";
}

//Round Robin Strategy
Participant* RoundRobinStrategy::selectParticipant(const WorkItem& item, const std::vector<Participant*>& candidates) {
    std::vector<Participant*> ok = eligible(item, candidates);
    if (ok.empty()) return nullptr;
    lastIndex = (lastIndex + 1) % (int)ok.size();
    return ok[lastIndex];
}

AssignmentStrategy* RoundRobinStrategy::clone()clone{
    return new RoundRobinStrategy(*this);
}
std::string RoundRobinStrategy::name()const{
    return "RoundRobin";
}