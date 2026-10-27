#include "AuditReportVisitor.h"
#include "ProcessDataStore.h"
#include "WorkflowMemento.h"

#include "tividy/state/States.h" 
#include "Command.h" 
#include <iostream>

void AuditReportVisitor::visitState(const WorkItemState& state) {
  
    std::cout << "System shifted into State: " << state.name() << "\n";
}

void AuditReportVisitor::visitHandler(const ChainOfResponsibilityElement& handler) {
    
    std::cout << " Pipeline step executed successfully.\n";
}

void AuditReportVisitor::visitCommand(const WorkflowCommand& command) { 
 
    std::cout << "[AUDIT LOG] ACTION TRIGGERED: " << command.name() << "\n"
              << "  -> Authorized By User  : " << command.getIssuedBy() << "\n"
              << "  -> Target Work Item ID: " << command.getWorkItemId() << "\n"
              << "  -> Execution Status    : " << command.getStatus() << "\n";
}

void AuditReportVisitor::visitProcessDataStore(ProcessDataStore& data) {
    std::cout << "--- COMPLIANCE TIMELINE START ---\n";
    for (const auto& memento : data.getBackupStorage()) {
        std::cout << "[" << memento->getTimestampString() << "] [COLD STORAGE] State: " << memento->getStateName() << " (v" << memento->getVersion() << ")\n";
    }
    for (const auto& memento : data.getLog()) {
        std::cout << "[" << memento->getTimestampString() << "] [HOT RUNTIME] State: " << memento->getStateName() << " (v" << memento->getVersion() << ")\n";
    }
    std::cout << "--- COMPLIANCE TIMELINE END ---\n";
}
