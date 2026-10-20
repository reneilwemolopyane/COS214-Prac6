#pragma once
#include <vector>
#include "tividy/core/Participant.h"
#include "tividy/mediator/Mediator.h"
class WorkItem;
/** @file Strategy.h  Strategy: AssignmentStrategy + concretes + WorkAssigner (Context). STUB */
class AssignmentStrategy {
public:
    virtual ~AssignmentStrategy() {}
    virtual Participant* select(const WorkItem& item, const std::vector<Participant*>& pool) = 0;
};
class RoleBasedStrategy : public AssignmentStrategy {
public: Participant* select(const WorkItem&, const std::vector<Participant*>&) { return 0; } };  // TODO
class ShortestQueueStrategy : public AssignmentStrategy {
public: Participant* select(const WorkItem&, const std::vector<Participant*>&) { return 0; } };  // TODO
class RoundRobinStrategy : public AssignmentStrategy {
public: Participant* select(const WorkItem&, const std::vector<Participant*>&) { return 0; } };  // TODO
class DataRuleStrategy : public AssignmentStrategy {
public: Participant* select(const WorkItem&, const std::vector<Participant*>&) { return 0; } };  // TODO

class WorkAssigner : public WorkflowColleague {
public:
    explicit WorkAssigner(AssignmentStrategy* s = 0) : strategy(s) {}
    void setStrategy(AssignmentStrategy*) {}   // TODO
    Participant* assign(const WorkItem&, const std::vector<Participant*>&) {
        return 0; }  // TODO
private:
    AssignmentStrategy* strategy;   // not owned
};
