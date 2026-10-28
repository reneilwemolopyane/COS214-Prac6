#pragma once
#include <string>
#include "tividy/core/Common.h"
#include "tividy/composite_prototype/WorkflowComponent.h"

class WorkflowInstance {
public:
    WorkflowInstance(const std::string& id, WorkflowComponent* root,
                     const DataMap& data, int definitionVersion)
        : id_(id), root_(root), data_(data), version_(definitionVersion), status_("Created") {}
    ~WorkflowInstance() { delete root_; }
    void start() { status_ = "Running"; }
    const std::string& getId() const { return id_; }
    std::string getStatus() const { return status_; }
private:
    std::string id_;
    WorkflowComponent* root_;
    DataMap data_;
    int version_;
    std::string status_;
};
