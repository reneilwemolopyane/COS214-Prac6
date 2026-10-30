#pragma once
#include <string>
#include "tividy/core/Common.h"
#include "tividy/composite_prototype/WorkflowComponent.h"

class WorkAssigner;
class Participant;
class WorkflowComponent;

class WorkflowInstance {
public:
    WorkflowInstance(const std::string& id,
                     WorkflowComponent* rootCopy,
                     const DataMap& d,
                     int version = 1,
                     WorkAssigner* assigner = 0,
                     const std::vector<Participant*>& pool = std::vector<Participant*>())
        : id_(id),
          root_(rootCopy),          // ← take ownership of the clone
          data_(d),
          version_(version),
          status_("Created")
    {
        (void)assigner;
        (void)pool;
    }

    ~WorkflowInstance() { delete root_; }
    void start() { status_ = "Running"; }
    void addDependency(const std::string&, const std::string&) {}
    const std::string& getId() const { return id_; }
    std::string getStatus() const { return status_; }
    int getVersion() const { return version_; }   // ← add, useful for future tests

private:
    std::string id_;
    WorkflowComponent* root_;
    DataMap data_;
    int version_;
    std::string status_;
};