#pragma once
#include <functional>
#include "WorkItemDecorator.h"

/** @brief Tracks how long the item has been running so the engine's timer
 *  can detect an overdue item. Time comes from an injected clock. */
class DeadlineDecorator : public WorkItemDecorator {
public:
    typedef std::function<long()> Clock;   ///< returns "now" in seconds

    /** @param w            wrapped component
     *  @param limitSeconds allowed time after start()
     *  @param c            clock (default: system time) */
    DeadlineDecorator(WorkflowComponent* w, long limitSeconds, Clock c = Clock());

    void start();

    /** @return true once the limit has passed since start() (false if not started) */
    bool isOverdue() const;

    /** @return seconds left, 0 if overdue or not started */
    long remaining() const;

    /** Clones the stack with fresh timing so cloned instances stay isolated. */
    WorkflowComponent* clone() const;
private:
    long now() const;
    long limit, startedAt;
    bool started;
    Clock clock;
};