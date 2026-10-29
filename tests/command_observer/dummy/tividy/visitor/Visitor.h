#pragma once
#include <string>
class WorkflowInstance;
class Stage;
class WorkItem;
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
    virtual void accept(WorkflowVisitor&) = 0;
};
