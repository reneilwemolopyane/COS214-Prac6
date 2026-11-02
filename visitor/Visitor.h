#ifndef VISITOR_H
#define VISITOR_H

#include <string>

#include "WorkflowElement.h"

class WorkflowInstance;
class Stage;
class WorkItem;
class ProcessDataStore;

/**
 * @brief Visitor interface for workflow elements and process data.
 *
 * Implementations can inspect supported workflow objects without adding
 * reporting or analysis logic to those objects.
 */
class WorkflowVisitor {
public:
    /** @brief Virtual destructor for polymorphic visitor implementations. */
    virtual ~WorkflowVisitor() {}

    /** @brief Visits a workflow instance. */
    virtual void visitInstance(WorkflowInstance& instance) = 0;

    /** @brief Visits a workflow stage. */
    virtual void visitStage(Stage& stage) = 0;

    /** @brief Visits a work item. */
    virtual void visitWorkItem(WorkItem& item) = 0;

    /**
     * @brief Visits the process data store.
     * @details The default implementation does nothing, allowing visitors
     *          that do not process stored data to omit an override.
     */
    virtual void visitProcessDataStore(ProcessDataStore& /*data*/) {}

    /** @return The report or summary produced by this visitor. */
    virtual std::string getReport() const = 0;
};

#endif