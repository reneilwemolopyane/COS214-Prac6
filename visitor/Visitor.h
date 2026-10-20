#pragma once
/** @file Visitor.h  Visitor: WorkflowElement + WorkflowVisitor + concrete visitors. STUB */
#include <string>
class WorkflowInstance; class Stage; class WorkItem;

class WorkflowVisitor {
public:
    virtual ~WorkflowVisitor() {}
    virtual void visitInstance(WorkflowInstance&) = 0;
    virtual void visitStage(Stage&) = 0;
    virtual void visitWorkItem(WorkItem&) = 0;
    virtual std::string getReport() const = 0;
};
class WorkflowElement {
public:
    virtual ~WorkflowElement() {}
    virtual void accept(WorkflowVisitor& v) = 0;
};
class BottleneckVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance&) {} void visitStage(Stage&) {} void visitWorkItem(WorkItem&) {}
    std::string getReport() const { return "TODO"; }
};
class SLAComplianceVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance&) {} void visitStage(Stage&) {} void visitWorkItem(WorkItem&) {}
    std::string getReport() const { return "TODO"; }
};
class AuditReportVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance&) {} void visitStage(Stage&) {} void visitWorkItem(WorkItem&) {}
    std::string getReport() const { return "TODO"; }
};
