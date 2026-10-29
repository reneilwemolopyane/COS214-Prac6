#pragma once
#include <string>
#include "tividy/visitor/Visitor.h"
class Participant;
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
    
        /** Command actions. Decorators intercept these so security, validation
     *  and audit also run when commands act on an item. */
    virtual void assign(Participant*) {}
    virtual void complete(const std::string& outcome) { complete(); }
    /** True if a deadline has passed. Decorators forward it; DeadlineDecorator answers it. */
    virtual bool isOverdue() const { return false; }
    /** WorkItem overrides this with a real rejection; other nodes just cancel. */
    virtual void reject(const std::string& reason) { (void)reason; cancel(); }

    void setParent(WorkflowComponent* p) { parent = p; }
    WorkflowComponent* getParent() const { return parent; }
    virtual void childFinished(WorkflowComponent& child) {} 

protected:
    WorkflowComponent* parent;
};