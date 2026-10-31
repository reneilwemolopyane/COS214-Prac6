#include "ValidationDecorator.h"
#include <stdexcept>

ValidationDecorator::ValidationDecorator(WorkflowComponent* w, Rule r)
    : WorkItemDecorator(w), rule(r) {}

void ValidationDecorator::complete() {
    if (rule && !rule())
        throw std::runtime_error("Validation failed for " + wrapped->getName());
    wrapped->complete();
}

void ValidationDecorator::complete(const std::string& outcome) {
    if (rule && !rule())
        throw std::runtime_error("Validation failed for " + wrapped->getName());
    wrapped->complete(outcome);
}

WorkflowComponent* ValidationDecorator::clone() const {
    return new ValidationDecorator(wrapped->clone(), rule);
}