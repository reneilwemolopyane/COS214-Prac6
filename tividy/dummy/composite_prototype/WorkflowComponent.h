#pragma once
#include <string>
#include "tividy/visitor/Visitor.h"
class WorkflowComponent : public WorkflowElement {
public:
    virtual ~WorkflowComponent() {}
    virtual void start() {}
    virtual std::string getName() const = 0;
};
