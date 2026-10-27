#ifndef BOTTLENECKVISITOR_H
#define BOTTLENECKVISITOR_H

#include "WorkflowVisitor.h"
#include <chrono>
#include <cstddef>
#include <iostream>
#include <unordered_map>

class WorkItemState;
class ApprovalHandler;
class ProcessDataStore;
class WorkflowCommand;

class BottleneckVisitor : public WorkflowVisitor {
public:
    void visitState(const WorkItemState& state) override;
    void visitHandler(const ApprovalHandler& handler) override;
    void visitProcessDataStore(ProcessDataStore& data) override;
    void visitCommand(const WorkflowCommand& command) override;

    void setStateEnteredAt(const WorkItemState& state,std::chrono::steady_clock::time_point enteredAt);
    void setHandlerExecutionTimes(const ApprovalHandler& handler,std::chrono::steady_clock::time_point startedAt,
        std::chrono::steady_clock::time_point endedAt);
    void setStateSlaSeconds(std::size_t seconds);
    void setHandlerSlaMilliseconds(std::size_t milliseconds);

private:
    std::unordered_map<const WorkItemState*, std::chrono::steady_clock::time_point> stateEnteredAt;
    std::unordered_map<const ApprovalHandler*,
        std::pair<std::chrono::steady_clock::time_point,
                  std::chrono::steady_clock::time_point>> handlerExecutionTimes;
    std::size_t stateSlaSeconds = 30;
    std::size_t handlerSlaMilliseconds = 500;

};

#endif