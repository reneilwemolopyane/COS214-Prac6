#ifndef BOTTLENECKVISITOR_H
#define BOTTLENECKVISITOR_H

#include "Visitor.h"
#include <chrono>
#include <cstddef>
#include <string>
#include <unordered_map>

class WorkflowInstance;
class Stage;
class WorkItem;
class ProcessDataStore;

class BottleneckVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance& instance) override;
    void visitStage(Stage& stage) override;
    void visitWorkItem(WorkItem& item) override;
    void visitProcessDataStore(ProcessDataStore& data) override;
    std::string getReport() const override { return report; }

    void setStageEnteredAt(const Stage& stage, std::chrono::steady_clock::time_point enteredAt);
    void setWorkItemEnteredAt(const WorkItem& item, std::chrono::steady_clock::time_point enteredAt);
    void setStageSlaSeconds(std::size_t seconds);
    void setWorkItemSlaSeconds(std::size_t seconds);

private:
    std::unordered_map<const Stage*, std::chrono::steady_clock::time_point> stageEnteredAt;
    std::unordered_map<const WorkItem*, std::chrono::steady_clock::time_point> workItemEnteredAt;
    std::size_t stageSlaSeconds = 30;
    std::size_t workItemSlaSeconds = 30;
    std::string report;

};

#endif