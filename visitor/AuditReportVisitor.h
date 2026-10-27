#ifndef AUDITREPORTVISITOR_H
#define AUDITREPORTVISITOR_H

#include "WorkflowVisitor.h"
#include <string>

class AuditReportVisitor : public WorkflowVisitor {
public:
    void visitState(const WorkItemState& state) override;
    void visitHandler(const ApprovalHandler& handler) override;
    void visitCommand(const WorkflowCommand& command) override;
    void visitProcessDataStore(ProcessDataStore& data) override;
};

#endif