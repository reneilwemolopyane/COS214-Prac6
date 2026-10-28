#pragma once
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"

class WorkItem : public WorkflowComponent {
public:
    WorkItem(const std::string& name, const std::string& role) : name_(name), role_(role) {}
    WorkflowComponent* clone() const override { return new WorkItem(name_, role_); }
private:
    std::string name_, role_;
};
