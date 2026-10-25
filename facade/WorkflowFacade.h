#ifndef WORKFLOWFACADE_H
#define WORKFLOWFACADE_H

#include <memory>
#include <string>
#include "DefinitionRepository.h"
#include "Common.h"
#include "WorkflowEngine.h"

/**
 * @file WorkflowFacade.h
 * @brief Single entry point for outside callers (UI, tests, demos).
 *
 * GoF role: Facade over DefinitionRepository, WorkflowDefinition, WorkflowInstance
 * and WorkflowEngine. It coordinates; it does not hold workflow logic itself.
 */
class WorkflowFacade {
public:
    /// The repository and engine must outlive the facade.
    WorkflowFacade(DefinitionRepository& repository, WorkflowEngine& engine);

    /**
     * @brief Store a definition so workflows can be started from it.
     * @throws std::invalid_argument if @p definition is null.
     */
    void registerDefinition(WorkflowDefinition* definition);

    /**
     * @brief Create, register and start a new independent instance.
     * @return The id of the new instance.
     * @throws std::out_of_range if no definition has this id.
     * @throws std::logic_error  if the definition is only a draft.
     */
    std::string startWorkflow(const std::string& definitionId, const DataMap& data);

    /**
     * @brief Current status of a running instance.
     * @throws std::out_of_range if the instance is unknown.
     */
    std::string getInstanceStatus(const std::string& instanceId);

private:
    DefinitionRepository& repository;
    
    WorkflowEngine& engine;
};

#endif
