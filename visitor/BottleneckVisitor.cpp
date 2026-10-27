#include "BottleneckVisitor.h"
#include "tividy/memento/ProcessDataStore.h"
#include "tividy/state/WorkItemState.h"
#include "tividy/command/Command.h"
#include "tividy/chain_of_responsibility/ApprovalHandler.h"

#include <chrono>
#include <iostream>

void BottleneckVisitor::visitState(const WorkItemState& state) {
    const auto entry = stateEnteredAt.find(&state);
    if (entry == stateEnteredAt.end()) {
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - entry->second).count();
    if (elapsed >= static_cast<long long>(stateSlaSeconds)) {
        std::cout << "[SLA ALERT] State " << state.name() << " idle for "
                  << elapsed << " seconds.\n";
    }
}

void BottleneckVisitor::visitHandler(const ApprovalHandler& handler) {
    const auto execution = handlerExecutionTimes.find(&handler);
    if (execution == handlerExecutionTimes.end()) {
        return;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        execution->second.second - execution->second.first).count();
    if (elapsed >= static_cast<long long>(handlerSlaMilliseconds)) {
        std::cout << "[SLA ALERT] Approval handler took " << elapsed << " ms.\n";
    }
}

void BottleneckVisitor::setHandlerExecutionTimes(const ApprovalHandler& handler,std::chrono::steady_clock::time_point startedAt,
    std::chrono::steady_clock::time_point endedAt) {
    handlerExecutionTimes[&handler] = std::make_pair(startedAt, endedAt);
}

void BottleneckVisitor::setStateEnteredAt(
    const WorkItemState& state, std::chrono::steady_clock::time_point enteredAt) {
    stateEnteredAt[&state] = enteredAt;
}

void BottleneckVisitor::setStateSlaSeconds(std::size_t seconds) {
    stateSlaSeconds = seconds;
}

void BottleneckVisitor::setHandlerSlaMilliseconds(std::size_t milliseconds) {
    handlerSlaMilliseconds = milliseconds;
}

void BottleneckVisitor::visitProcessDataStore(ProcessDataStore& data) {
    if (data.IspastThreshold()) {
        std::cout << "Memento log threshold exceeded: "
                  << data.getLog().size() << '\n';
        data.backupstorage();
    }
}

void BottleneckVisitor::visitCommand(const WorkflowCommand& command) {
    (void)command;
}

