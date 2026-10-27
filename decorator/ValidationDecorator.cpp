#include "ValidationDecorator.h"
#include <stdexcept>

ValidationDecorator::ValidationDecorator(WorkflowComponent* w, Rule r)
    : WorkItemDecorator(w), rule(r) {}

void ValidationDecorator::complete() {
    if (rule && !rule())
        throw std::runtime_error("Validation failed for " + wrapped->getName());
    wrapped->complete();
}

WorkflowComponent* ValidationDecorator::clone() const {
    return new ValidationDecorator(wrapped->clone(), rule);
}