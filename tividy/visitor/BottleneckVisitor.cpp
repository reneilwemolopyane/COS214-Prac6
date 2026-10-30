#include "tividy/visitor/BottleneckVisitor.h"
#include "tividy/memento/ProcessDataStore.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/composite_prototype/WorkflowComponent.h"
#include <chrono>

void BottleneckVisitor::visitInstance(WorkflowInstance& instance) {
    report += "Checking workflow instance " + instance.getId() + "\n";
}

void BottleneckVisitor::visitStage(Stage& stage) {
    report += "Checking stage " + stage.getName() + "\n";
    const auto entry = stageEnteredAt.find(&stage);
    if (entry != stageEnteredAt.end()) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - entry->second).count();
        if (elapsed >= static_cast<long long>(stageSlaSeconds)) {
            report += "[BOTTLENECK] Stage " + stage.getName() + " exceeded its time limit.\n";
        }
    }
}

void BottleneckVisitor::visitDecorator(WorkflowComponent& component) {
    if (!component.isOverdue()) return;
    // each stacked layer calls this, so report an item only once (keyed by name)
    if (!overdueReported.insert(component.getName()).second) return;
    report += "[BOTTLENECK] Work item " + component.getName() +
              " exceeded its deadline.\n";
}

void BottleneckVisitor::visitWorkItem(WorkItem& item) {
    report += "Checking work item " + item.getId() + " (" + item.getStateName() + ")\n";
    const auto entry = workItemEnteredAt.find(&item);
    if (entry != workItemEnteredAt.end()) {
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - entry->second).count();
        if (elapsed >= static_cast<long long>(workItemSlaSeconds)) {
            report += "[BOTTLENECK] Work item " + item.getId() + " exceeded its time limit.\n";
        }
    }
}

void BottleneckVisitor::setStageEnteredAt(
    const Stage& stage, std::chrono::steady_clock::time_point enteredAt) {
    stageEnteredAt[&stage] = enteredAt;
}

void BottleneckVisitor::setWorkItemEnteredAt(
    const WorkItem& item, std::chrono::steady_clock::time_point enteredAt) {
    workItemEnteredAt[&item] = enteredAt;
}

void BottleneckVisitor::setStageSlaSeconds(std::size_t seconds) {
    stageSlaSeconds = seconds;
}

void BottleneckVisitor::setWorkItemSlaSeconds(std::size_t seconds) {
    workItemSlaSeconds = seconds;
}

void BottleneckVisitor::visitProcessDataStore(ProcessDataStore& data) {
    if (data.IspastThreshold()) {
        report += "Memento log threshold exceeded: " +
                  std::to_string(data.getLog().size()) + "\n";
        data.backupstorage();
    }
}

