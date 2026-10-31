#include "DeadlineDecorator.h"
#include <ctime>

DeadlineDecorator::DeadlineDecorator(WorkflowComponent* w, long limitSeconds, Clock c)
    : WorkItemDecorator(w), limit(limitSeconds), startedAt(0), started(false), clock(c) {}

long DeadlineDecorator::now() const {
    return clock ? clock() : static_cast<long>(std::time(0));
}

void DeadlineDecorator::start() {
    wrapped->start();
    startedAt = now();
    started = true;
}

bool DeadlineDecorator::isOverdue() const {
    return started && (now() - startedAt) > limit;
}

long DeadlineDecorator::remaining() const {
    if (!started) return 0;
    long left = limit - (now() - startedAt);
    return left > 0 ? left : 0;
}

WorkflowComponent* DeadlineDecorator::clone() const {
    return new DeadlineDecorator(wrapped->clone(), limit, clock);
}