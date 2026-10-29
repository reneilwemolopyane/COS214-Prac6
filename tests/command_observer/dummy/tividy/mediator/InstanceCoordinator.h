#pragma once
class WorkItem;
class WorkAssigner;
class WorkflowSubject;
class InstanceCoordinator {
public:
    InstanceCoordinator(WorkAssigner*, WorkflowSubject*) {}
    void registerItem(WorkItem*) {}
    void start() {}
};
