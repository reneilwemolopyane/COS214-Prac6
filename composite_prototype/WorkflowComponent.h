#pragma once
#include <string>
#include "tividy/visitor/Visitor.h"

class WorkflowComponent : public WorkflowElement {
public:
    WorkflowComponent() : parent(nullptr) {}
    virtual ~WorkflowComponent() {}

    
    virtual void start() = 0;
    virtual void complete() = 0;
    virtual void cancel() = 0;
    virtual bool isComplete() const = 0;
    virtual WorkflowComponent* clone() const = 0;   
    virtual std::string getName() const = 0;
    
    
    virtual void activate() = 0;                    
    virtual bool isFinished() const = 0;         
    virtual bool isFailed() const = 0;             
    
    
    void setParent(WorkflowComponent* p) { parent = p; }
    WorkflowComponent* getParent() const { return parent; }
    virtual void childFinished(WorkflowComponent& child) {} 

protected:
    WorkflowComponent* parent;
};