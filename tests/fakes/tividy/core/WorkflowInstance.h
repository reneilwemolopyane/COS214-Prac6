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
                     const std::vector<Participant*>& pool = std::vector<Participant*>()){} 
    ~WorkflowInstance() { delete root_; }
    void start() { status_ = "Running"; }
    void addDependency(const std::string&, const std::string&) {}
    const std::string& getId() const { return id_; }
    std::string getStatus() const { return status_; }
private:
    std::string id_;
    WorkflowComponent* root_;
    DataMap data_;
    int version_;
    std::string status_;
};
