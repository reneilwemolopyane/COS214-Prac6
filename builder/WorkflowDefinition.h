#pragma once
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/core/Common.h"
class WorkflowInstance;
/** @brief Product (Builder) + Prototype client. STUB */
class WorkflowDefinition {
public:
    WorkflowDefinition(const std::string& id = "", int version = 1) : id(id), version(version), root(0) {}
    void setRoot(WorkflowComponent*) {}                     // TODO
    WorkflowInstance* createInstance(const DataMap& data);  // TODO: see src/builder
    std::string getId() const { return id; }
    int getVersion() const { return version; }
    DataMap rules;
private:
    std::string id; int version; WorkflowComponent* root;
};
