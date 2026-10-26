#pragma once
#include <vector>
#include "Participant.h"
#include "Mediator.h"
#include <string>
class WorkItem;
/** @file AssignmentStrategy.h  Strategy: AssignmentStrategy + concretes + WorkAssigner (Context). STUB */
class AssignmentStrategy {
  public:
    virtual ~AssignmentStrategy() {}
    virtual Participant* selectParticipant(const WorkItem& item, const std::vector<Participant*>&) = 0;
    virtual AssignmentStrategy* clone()const = 0;
    virtual std::string name()const = 0;
};
class RoleBasedStrategy : public AssignmentStrategy{
  public: 
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
}
class ShortestQueueStrategy : public AssignmentStrategy{
  public: 
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
}
class RoundRobinStrategy : public AssignmentStrategy{
  private:
    int lastIndex;
  public:
    RoundRobinStrategy() : lastIndex(-1){}
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
}
class DataRuleStrategy : public AssignmentStrategy{
  private:
    std::string dataKey; //e.g "claimtype" or "faculty"
  public: 
    Participant* selectParticipant(const WorkItem&, const std::vector<Participant*>&)override;
    AssignmentStrategy* clone()const override;
    std::string name()const;
}

std::vector<Participant*> AssigmentStrategy::eligible(const WorkItem& item, const std::vector<Participant*>& candidates)const{
    std::vector<Participant*> result;
    for (int i = 0; i < (int)candidates.size(); i++)
        if (candidates[i]->hasRole(item.getRequiredRole()))
            result.push_back(candidates[i]);
    return result;
}


class WorkAssigner : public WorkflowColleague {
public:
    WorkAssigner(AssignmentStrategy* s = nullptr) : strategy(s){}
    void setStrategy(AssignmentStrategy*){
        strategy = s;
    }  
    Participant* assign(const WorkItem&, const std::vector<Participant*>&){
        if(strategy == nullptr) return nullptr;
        return strategy->selectParticipant(item, pool);
    }
private:
    AssignmentStrategy* strategy;   // not owned
};
