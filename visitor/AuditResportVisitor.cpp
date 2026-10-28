#include "AuditReportVisitor.h"
#include "memento/ProcessDataStore.h"
#include "memento/Memento.h"
#include "composite_prototype/Stage.h"
#include "composite_prototype/WorkItem.h"
#include "core/WorkflowInstance.h"

#include <algorithm>
#include <iostream>
#include <vector>

void AuditReportVisitor::visitInstance(WorkflowInstance& instance) {
    report << "Workflow instance " << instance.getId()
           << " (version " << instance.getVersion() << ")\n";
}

void AuditReportVisitor::visitStage(Stage& stage) {
    report << "Stage: " << stage.getName() << '\n';
}

void AuditReportVisitor::visitWorkItem(WorkItem& item) {
    report << "Work item " << item.getId() << ": " << item.getName()
           << " (" << item.getStateName() << ")\n";
}

void AuditReportVisitor::visitProcessDataStore(ProcessDataStore& data) {
    report << "--- COMPLIANCE TIMELINE START ---\n";
    std::vector<const InstanceMemento*> history;
    history.reserve(data.getBackupStorage().size() + data.getLog().size());
    for (const auto* memento : data.getBackupStorage()) {
        history.push_back(memento);
    }
    for (const auto* memento : data.getLog()) {
        history.push_back(memento);
    }
    std::stable_sort(history.begin(), history.end(),
                     [](const InstanceMemento* left, const InstanceMemento* right) {
                         return left->getTimestamp() < right->getTimestamp();
                     });
    for (const InstanceMemento* memento : history) {
        report << '[' << memento->getTimestampString() << "] Snapshot: "
               << memento->getStateSnapshot() << " (version "
               << memento->getVersion() << ")\n";
    }
    report << "--- COMPLIANCE TIMELINE END ---\n";
}
