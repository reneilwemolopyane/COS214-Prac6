#include "AuditReportVisitor.h"
#include "tividy/memento/ProcessDataStore.h"
#include "tividy/memento/WorkflowMemento.h"

#include "tividy/state/WorkItemState.h"
#include "tividy/command/Command.h"
#include "tividy/chain_of_responsibility/ApprovalHandler.h"

#include <algorithm>
#include <iostream>
#include <vector>

void AuditReportVisitor::visitState(const WorkItemState& state) {
  
    std::cout << "System shifted into State: " << state.name() << "\n";
}

void AuditReportVisitor::visitHandler(const ApprovalHandler&) {
    
    std::cout << " Pipeline step executed successfully.\n";
}

void AuditReportVisitor::visitCommand(const WorkflowCommand& command) { 
 
    std::cout << "[AUDIT LOG] ACTION TRIGGERED: " << command.name() << "\n" << "  -> Authorized By User  : " << command.getIssuedBy() << "\n"
    << "  -> Target Work Item ID: " << command.getWorkItemId() << "\n"<< "  -> Execution Status    : " << command.getStatus() << "\n";
}

void AuditReportVisitor::visitProcessDataStore(ProcessDataStore& data) {
    std::cout << "--- COMPLIANCE TIMELINE START ---\n";
    std::vector<const WorkflowMemento*> history;
    history.reserve(data.getBackupStorage().size() + data.getLog().size());
    for (const auto* memento : data.getBackupStorage()) {
        history.push_back(memento);
    }
    for (const auto* memento : data.getLog()) {
        history.push_back(memento);
    }
    std::stable_sort(history.begin(), history.end(),
                     [](const WorkflowMemento* left, const WorkflowMemento* right) {
                         return left->getTimestamp() < right->getTimestamp();
                     });
    for (const WorkflowMemento* memento : history) {
        std::cout << "[" << memento->getTimestampString() << "] State: "
                  << memento->getStateName() << " (" << memento->getVersion() << ")\n";
    }
    std::cout << "--- COMPLIANCE TIMELINE END ---\n";
}
