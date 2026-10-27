#include "PriorityDecorator.h"
#include <sstream>

PriorityDecorator::PriorityDecorator(WorkflowComponent* w, int p)
    : WorkItemDecorator(w), priority(p) {}

std::string PriorityDecorator::displayName() const {
    std::ostringstream os;
    os << "[P" << priority << "] " << wrapped->getName();
    return os.str();
}

int PriorityDecorator::getPriority() const { 
    return priority; 
}

long PriorityDecorator::adjustDeadline(long baseSeconds) const {
    return baseSeconds / (priority < 1 ? 1 : priority);
}

WorkflowComponent* PriorityDecorator::clone() const {
    return new PriorityDecorator(wrapped->clone(), priority);
}