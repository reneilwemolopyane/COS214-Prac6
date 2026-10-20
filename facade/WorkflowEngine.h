#pragma once
#include <vector>
#include "tividy/core/WorkflowInstance.h"
/** @brief Registers/starts instances. STUB */
class WorkflowEngine {
public:
    void registerInstance(WorkflowInstance*) {}   // TODO
    void start(WorkflowInstance*) {}              // TODO
private: std::vector<WorkflowInstance*> instances;
};
