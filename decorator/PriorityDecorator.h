#pragma once
#include <string>
#include "WorkItemDecorator.h"

/** @brief Marks the item with a priority level. getName() stays unchanged so
 *  lookups by name keep working; use displayName() for screens and reports. */
class PriorityDecorator : public WorkItemDecorator {
public:
    /** @param w wrapped component
     *  @param p priority level (higher = more urgent) */
    PriorityDecorator(WorkflowComponent* w, int p = 1);

    /** @return the name with the priority in front, e.g. "[P3] Inspect damage" */
    std::string displayName() const;
    int getPriority() const;

    /** Shortens a deadline according to priority.
     *  @param baseSeconds the normal deadline
     *  @return baseSeconds divided by the priority (priority below 1 counts as 1) */
    long adjustDeadline(long baseSeconds) const;

    WorkflowComponent* clone() const;
private:
    int priority;
};