#pragma once
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"

class Participant;   // harmless if WorkflowComponent.h already declares it

/** @file WorkItemDecorator.h
 *  @brief Decorator: layers extra behaviour onto a work item without
 *  subclassing for every combination. */

/** @brief Decorator base. Owns the wrapped component and forwards every call.
 *  Abstract: concrete decorators supply clone(). */
class WorkItemDecorator : public WorkflowComponent {
public:
    /** @param w component to wrap (ownership transfers to the decorator) */
    explicit WorkItemDecorator(WorkflowComponent* w);
    virtual ~WorkItemDecorator();

    void start();
    void complete();
    void cancel();
    bool isComplete() const;
    std::string getName() const;
    void accept(WorkflowVisitor& v);

    /** Command actions. Forwarded here so concrete decorators can intercept
     *  them: commands call these on the outermost decorator. */
    void assign(Participant* p) override;
    void complete(const std::string& outcome) override;

    void activate() override;
    bool isFinished() const override;
    bool isFailed() const override;

    /** The wrapped item reports to this decorator, which relays the
     *  notification to its own parent as the decorated item. */
    void childFinished(WorkflowComponent& child) override;

protected:
    WorkflowComponent* wrapped;   ///< Owned. Deep-copied by clone().

private:
    WorkItemDecorator(const WorkItemDecorator&) = delete;
    WorkItemDecorator& operator=(const WorkItemDecorator&) = delete;
};