#include "AssignmentStrategy.h"
#include "../composite_state/WorkItem.h"
#include "../core/Participant.h"

AssignmentStrategy::~AssignmentStrategy(){}

std::vector<Participant*> AssignmentStrategy::eligible(const WorkItem& item, const std::vector<Participant*>& candidates)const{
    std::vector<Participant*> ok;
    for (size_t i = 0; i < candidates.size(); i++){
        if (candidates[i]->hasRole(item.getRole())){
            ok.push_back(candidates[i]);
        }
    }
    return ok;
}

//Role based Strategy
Participant* RoleBasedStrategy::selectParticipant(const WorkItem&, const std::vector<Participant*>& candidates){
    std::vector<Participant*> ok = eligible(item, candidates);
    return ok.empty() ? nullptr : ok[0];
}
AssignmentStrategy* RoleBasedStrategy::clone()const{
    return new RoleBasedStrategy(*this);
}
std::string RoleBasedStrategy::name()const{
    return "RoleBased";
}

//Shortest Queue Strategy
Participant* ShortestQueueStrategy::selectParticipant(const WorkItem&, const std::vector<Participant*>&){
    std::vector<Participant*>ok = eligible(item, candidates);
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

AssignmentStrategy* RoundRobinStrategy::clone()const{
    return new RoundRobinStrategy(*this);
}
std::string RoundRobinStrategy::name()const{
    return "RoundRobin";
}

//Data rule
void DataRuleStrategy::addMapping(const std::string& value, const std::string& role){
    roleByValue[value] = role;
}

Participant* DataRuleStrategy::selectParticipant(const WorkItem& item, const std::vector<Participant*>& candidates){
    std::map<std::string, std::string>::const_iterator it = roleByValue.find(item.getData(dataKey));
    if (it == roleByValue.end()) return nullptr;
    for (size_t i = 0; i < candidates.size(); i++){
        if (candidates[i]->hasRole(it->second)) return candidates[i];
    }
    return nullptr;
}
AssignmentStrategy* DataRuleStrategy::clone()const{
    return new DataRuleStrategy(*this);
}

std::string DataRuleStrategy::name() const{
    return "DataRule(" + dataKey + ")";
}