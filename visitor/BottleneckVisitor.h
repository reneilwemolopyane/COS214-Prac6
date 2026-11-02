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

/**
 * @brief Reports workflow progress and detects stages or items exceeding their
 *        configured time limits.
 *
 * Entry times must be registered for timed checks to apply. Visiting the
 * process data store also checks whether its memento log has passed its
 * threshold and backs it up when necessary.
 */
class BottleneckVisitor : public WorkflowVisitor {
public:
    /** @brief Reports the workflow instance being checked. */
    void visitInstance(WorkflowInstance& instance) override;

    /** @brief Reports a stage and flags it if its registered time limit elapsed. */
    void visitStage(Stage& stage) override;

    /** @brief Reports a work item and flags it if its registered time limit elapsed. */
    void visitWorkItem(WorkItem& item) override;

    /** @brief Checks the memento log threshold and backs up the store if exceeded. */
    void visitProcessDataStore(ProcessDataStore& data) override;

    /** @return The bottleneck and progress report accumulated so far. */
    std::string getReport() const override { return report; }

    /**
     * @brief Registers the time at which a stage was entered.
     * @param stage Stage whose elapsed time will be checked.
     * @param enteredAt Monotonic-clock time when the stage was entered.
     */
    void setStageEnteredAt(const Stage& stage, std::chrono::steady_clock::time_point enteredAt);

    /**
     * @brief Registers the time at which a work item was entered.
     * @param item Work item whose elapsed time will be checked.
     * @param enteredAt Monotonic-clock time when the work item was entered.
     */
    void setWorkItemEnteredAt(const WorkItem& item, std::chrono::steady_clock::time_point enteredAt);

    /** @brief Sets the stage time limit used for bottleneck checks, in seconds. */
    void setStageSlaSeconds(std::size_t seconds);

    /** @brief Sets the work-item time limit used for bottleneck checks, in seconds. */
    void setWorkItemSlaSeconds(std::size_t seconds);

private:
    std::unordered_map<const Stage*, std::chrono::steady_clock::time_point> stageEnteredAt;
    std::unordered_map<const WorkItem*, std::chrono::steady_clock::time_point> workItemEnteredAt;
    std::size_t stageSlaSeconds = 30;
    std::size_t workItemSlaSeconds = 30;
    std::string report;

};

#endif