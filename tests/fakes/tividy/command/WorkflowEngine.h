#pragma once
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include "tividy/core/WorkflowInstance.h"

class WorkflowEngine {
public:
    ~WorkflowEngine() {
        for (std::map<std::string, WorkflowInstance*>::iterator it = instances_.begin();
             it != instances_.end(); ++it) delete it->second;
    }
    // Takes ownership on success. Returns false (and does NOT take ownership) if
    // the instance is null or its id is already registered.
    bool registerInstance(WorkflowInstance* instance) {
        if (instance == 0 || instances_.count(instance->getId()) != 0) return false;
        instances_[instance->getId()] = instance;
        return true;
    }
    bool start(WorkflowInstance* instance) {
        if (instance == 0) return false;
        instance->start();
        return true;
    }
    std::string getStatus(const std::string& instanceId) const {
        std::map<std::string, WorkflowInstance*>::const_iterator it = instances_.find(instanceId);
        if (it == instances_.end()) throw std::out_of_range("Instance not found: " + instanceId);
        return it->second->getStatus();
    }
    // Test helpers only, not part of the contract with the real engine.
    WorkflowInstance* findInstance(const std::string& id) const {
        std::map<std::string, WorkflowInstance*>::const_iterator it = instances_.find(id);
        return it == instances_.end() ? 0 : it->second;
    }
    std::size_t instanceCount() const { return instances_.size(); }
private:
    std::map<std::string, WorkflowInstance*> instances_;
};
