#ifndef AUDITREPORTVISITOR_H
#define AUDITREPORTVISITOR_H

#include "Visitor.h"
#include <sstream>
#include <string>

/**
 * @brief Builds an audit report from workflow objects and stored history.
 *
 * The report includes instance, stage, and work-item details, followed by a
 * timestamp-ordered compliance timeline when the process data store is visited.
 */
class AuditReportVisitor : public WorkflowVisitor {
public:
    /** @brief Adds the instance identifier and version to the report. */
    void visitInstance(WorkflowInstance& instance) override;

    /** @brief Adds the stage name to the report. */
    void visitStage(Stage& stage) override;

    /** @brief Adds the work-item identifier, name, and state to the report. */
    void visitWorkItem(WorkItem& item) override;

    /** @brief Adds the store's timestamp-ordered memento history to the report. */
    void visitProcessDataStore(ProcessDataStore& data) override;

    /** @return The audit report accumulated so far. */
    std::string getReport() const override { return report.str(); }

private:
    std::ostringstream report;
};

#endif