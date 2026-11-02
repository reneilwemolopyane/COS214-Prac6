#pragma once
#include <functional>
#include <string>
#include "WorkItemDecorator.h"

/** @brief Blocks completion unless a configurable rule passes. */
class ValidationDecorator : public WorkItemDecorator {
public:
    typedef std::function<bool()> Rule;

    /** @param w wrapped component
     *  @param r rule that must return true before completion (none = always passes) */
    ValidationDecorator(WorkflowComponent* w, Rule r = Rule());

    /** @throws std::runtime_error if the rule fails */
    void complete();
    void complete(const std::string& outcome) override;
    WorkflowComponent* clone() const;
private:
    Rule rule;
};