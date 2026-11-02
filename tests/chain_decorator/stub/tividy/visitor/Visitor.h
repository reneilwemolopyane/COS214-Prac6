#pragma once
// Temporary stand-in until visitor/Visitor.h lands on dev. Delete this folder after.
class WorkflowVisitor {
public:
    virtual ~WorkflowVisitor() {}
};
class WorkflowElement {
public:
    virtual ~WorkflowElement() {}
    virtual void accept(WorkflowVisitor& v) = 0;
};
