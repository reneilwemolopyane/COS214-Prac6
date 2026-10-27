#ifndef WORKFLOWELEMENT_H
#define WORKFLOWELEMENT_H

class WorkflowElement {
public:
    virtual ~WorkflowElement() {}
    virtual void accept(WorkflowVisitor& v) = 0;
};

#endif