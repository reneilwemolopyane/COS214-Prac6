#pragma once
#include "DefinitionRepository.h"

#include "WorkflowEngine.h"

/** @brief Facade: single entry point for UI. STUB */

class WorkflowFacade {

public:
    WorkflowFacade(DefinitionRepository& r, WorkflowEngine& e) : repo(r), engine(e);

    std::string startWorkflow(const std::string& definitionId, const DataMap& data);  // see src/

    std::string getInstanceStatus(std::string instanceId);

private: 

    DefinitionRepository& repo;
    
    WorkflowEngine& engine;
};
