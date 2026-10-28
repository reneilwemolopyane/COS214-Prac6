#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include "tividy/composite_prototype/WorkflowComponent.h"

class Stage : public WorkflowComponent {
public:
    explicit Stage(const std::string& name) : name_(name) {}
    ~Stage() { for (std::size_t i = 0; i < children_.size(); ++i) delete children_[i]; }
    void add(WorkflowComponent* child) { children_.push_back(child); }
    WorkflowComponent* clone() const override { return new Stage(name_); }
private:
    std::string name_;
    std::vector<WorkflowComponent*> children_;
};
