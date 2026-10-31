#pragma once
#include <string>
#include <vector>
#include <utility>
#include "tividy/visitor/Visitor.h"
class Participant;
class WorkItem;
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
    virtual void complete(const std::string& /*outcome*/) { complete(); }
    
    void setParent(WorkflowComponent* p) { parent = p; }
    WorkflowComponent* getParent() const { return parent; }
    virtual void childFinished(WorkflowComponent& /*child*/) {} 

    /** The leaf work item inside this component (decorators forward to what they wrap), or null for groups. */
    virtual WorkItem* underlyingItem() { return 0; }
    /** Collects (leaf item, outermost component) pairs so the engine can talk to decorated items
     *  polymorphically. Stages recurse; a leaf or decorator reports itself. */
    virtual void collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& /*out*/) {}
    /** @return this as a Stage (groups) or null (leaves): replaces dynamic_cast when walking the tree. */
    virtual class Stage* asStage() { return 0; }
    virtual const class Stage* asStage() const { return 0; }
    /** Deadline in seconds after decorators such as Priority have shortened it. */
    virtual long adjustDeadline(long baseSeconds) const { return baseSeconds; }
    /** Adds (earlier item id, later item id) pairs implied by this component's structure:
     *  a SEQUENTIAL stage runs each child only after the previous child has finished. */
    virtual void collectOrdering(std::vector<std::pair<std::string, std::string> >& /*out*/) {}
    /** Replaces a direct child with a component that wraps it (used to attach decorators). */
    virtual bool replaceChild(WorkflowComponent* /*oldChild*/, WorkflowComponent* /*wrapper*/) { return false; }

protected:
    WorkflowComponent* parent;
};