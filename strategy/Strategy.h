#pragma once
#include <vector>
#include "Participant.h"
#include "Mediator.h"
#include <string>
class WorkItem;
/** @file Strategy.h  Strategy: AssignmentStrategy + concretes + WorkAssigner (Context). STUB */
class AssignmentStrategy {
  public:
    virtual ~Strategy() {}
    virtual Participant* select(const WorkItem& item, const std::vector<Participant*>&) = 0;
    virtual Strategy* clone()const = 0;
    virtual std::string name()const = 0;
};
class RoleBasedStrategy : public AssignmentStrategy{
  public: 
    Participant* select(const WorkItem&, const std::vector<Participant*>&)override;
    Strategy* clone()const override;
    std::string name()const;
}
class ShortestQueueStrategy : public AssignmentStrategy{
  public: 
    Participant* select(const WorkItem&, const std::vector<Participant*>&)override;
    Strategy* clone()const override;
    std::string name()const;
}
class RoundRobinStrategy : public AssignmentStrategy{
  private:
    int lastIndex;
  public:
    RoundRobinStrategy() : lastIndex(-1){}
    Participant* select(const WorkItem&, const std::vector<Participant*>&)override;
    Strategy* clone()const override;
    std::string name()const;
}
class DataRuleStrategy : public AssignmentStrategy{
  private:
    std::string dataKey; //e.g "claimtype" or "faculty"
  public: 
    Participant* select(const WorkItem&, const std::vector<Participant*>&)override;
    Strategy* clone()const override;
    std::string name()const;
}


class WorkAssigner : public WorkflowColleague {
public:
    explicit WorkAssigner(AssignmentStrategy* s = 0) : strategy(s) {}
    void setStrategy(AssignmentStrategy*) {}   // TODO
    Participant* assign(const WorkItem&, const std::vector<Participant*>&) {
        return 0; }  // TODO
private:
    AssignmentStrategy* strategy;   // not owned
};
