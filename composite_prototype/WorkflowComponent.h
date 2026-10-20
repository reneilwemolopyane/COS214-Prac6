#pragma once
/** @file WorkflowComponent.h  Composite/Prototype/Decorator component interface. STUB */
#include <string>
#include "tividy/visitor/Visitor.h"
class WorkflowComponent : public WorkflowElement {
public:
    virtual ~WorkflowComponent() {}
    virtual void start() = 0;
    virtual void complete() = 0;
    virtual void cancel() = 0;
    virtual bool isComplete() const = 0;
    virtual WorkflowComponent* clone() const = 0;   // Prototype: deep copy, fresh state
    virtual std::string getName() const = 0;
};
