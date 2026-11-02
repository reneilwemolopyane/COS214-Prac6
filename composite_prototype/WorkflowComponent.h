#pragma once
#include <string>
#include <vector>
#include <utility>
#include "tividy/visitor/Visitor.h"
class Participant;
class WorkItem;

/**
 * @brief Common interface for workflow leaves, groups, and decorators.
 *
 * Components form an owned tree. Groups coordinate children, while leaves
 * perform work; decorators can forward these operations while adding behavior.
 */
class WorkflowComponent : public WorkflowElement {
public:
    /** @brief Creates a component with no parent. */
    WorkflowComponent() : parent(nullptr) {}
    /** @brief Virtual destructor for component implementations. */
    virtual ~WorkflowComponent() {}

    /** @brief Starts execution of this component. */
    virtual void start() = 0;
    /** @brief Completes this component when its semantics permit. */
    virtual void complete() = 0;
    /** @brief Cancels this component. */
    virtual void cancel() = 0;
    /** @return Whether this component completed successfully. */
    virtual bool isComplete() const = 0;
    /** @return A heap-allocated copy of this component; caller owns the result. */
    virtual WorkflowComponent* clone() const = 0;   
    /** @return The component's display name. */
    virtual std::string getName() const = 0;
    
    /** @brief Activates the component for execution. */
    virtual void activate() = 0;                    
    /** @return Whether execution has reached a terminal state. */
    virtual bool isFinished() const = 0;         
    /** @return Whether this component or any contained work has failed. */
    virtual bool isFailed() const = 0;             
    
    /**
     * @brief Assigns a participant to this component.
     * @details Decorators may intercept command actions so validation and
     *          auditing also apply when a command acts on an item.
     */
    virtual void assign(Participant*) {}
    /**
     * @brief Completes this component with an outcome.
     * @details The default implementation delegates to complete().
     */
    virtual void complete(const std::string& /*outcome*/) { complete(); }
    
    /** @brief Sets the parent group of this component. */
    void setParent(WorkflowComponent* p) { parent = p; }
    /** @return The parent group, or nullptr when this is a root. */
    WorkflowComponent* getParent() const { return parent; }
    /** @brief Notifies the component that a child has finished. */
    virtual void childFinished(WorkflowComponent& /*child*/) {} 

    /** @return The underlying work item, or nullptr for a group. */
    virtual WorkItem* underlyingItem() { return 0; }
    /**
     * @brief Collects work items together with their outermost components.
     * @details Appends (work item, component) pairs to the supplied vector.
     */
    virtual void collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& /*out*/) {}
    /** @return This component as a Stage, or nullptr when it is not a group. */
    virtual class Stage* asStage() { return 0; }
    /** @copydoc asStage() */
    virtual const class Stage* asStage() const { return 0; }
    /** @brief Adjusts a deadline after decorator-specific policy is applied. */
    virtual long adjustDeadline(long baseSeconds) const { return baseSeconds; }
    /**
     * @brief Collects item ordering constraints implied by this component.
     * @details Appends (earlier item ID, later item ID) pairs to the supplied vector.
     */
    virtual void collectOrdering(std::vector<std::pair<std::string, std::string> >& /*out*/) {}
    /**
     * @brief Replaces a child with a wrapper component.
     * @return true if a matching child was replaced.
     */
    virtual bool replaceChild(WorkflowComponent* /*oldChild*/, WorkflowComponent* /*wrapper*/) { return false; }

protected:
    /** Parent group; non-owning pointer. */
    WorkflowComponent* parent;
};