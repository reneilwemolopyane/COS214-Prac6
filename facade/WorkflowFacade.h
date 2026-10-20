#pragma once
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/facade/WorkflowEngine.h"
/** @brief Facade: single entry point for UI. STUB */
class WorkflowFacade {
public:
    WorkflowFacade(DefinitionRepository& r, WorkflowEngine& e) : repo(r), engine(e) {}
    std::string startWorkflow(const std::string& definitionId, const DataMap& data);  // see src/
private: WorkflowFacade& operator=(const WorkflowFacade&);
    DefinitionRepository& repo; WorkflowEngine& engine;
};
