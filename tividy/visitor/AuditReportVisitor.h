#ifndef AUDITREPORTVISITOR_H
#define AUDITREPORTVISITOR_H

#include "tividy/visitor/Visitor.h"
#include <sstream>
#include <string>

class AuditReportVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance& instance) override;
    void visitStage(Stage& stage) override;
    void visitWorkItem(WorkItem& item) override;
    void visitProcessDataStore(ProcessDataStore& data) override;
    std::string getReport() const override { return report.str(); }

private:
    std::ostringstream report;
};

#endif