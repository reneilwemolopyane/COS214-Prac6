#pragma once
class WorkItem;
class WorkAssigner;
class WorkflowSubject;
class Participant; 
class InstanceCoordinator {
public:
    InstanceCoordinator(WorkAssigner*, WorkflowSubject*) {}
    void registerItem(WorkItem*) {}
    void start() {}
    void addDependency(const std::string& , const std::string& ) {}
    void addParticipant(Participant* ) {}
};
