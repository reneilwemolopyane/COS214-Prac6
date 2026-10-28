#pragma once
class WorkflowComponent {
public:
    virtual ~WorkflowComponent() {}
    virtual WorkflowComponent* clone() const = 0;
};