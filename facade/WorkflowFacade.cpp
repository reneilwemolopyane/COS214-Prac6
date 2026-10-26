#include "WorkflowFacade.h"
#include <stdexcept>

WorkflowFacade::WorkflowFacade(DefinitionRepository& repository, WorkflowEngine& engine)
    : repository(repository), engine(engine) {}

void WorkflowFacade::registerDefinition(WorkflowDefinition* definition) {

    repository.save(std::move(definition));
}

std::string WorkflowFacade::startWorkflow(const std::string& definitionId, const DataMap& data) {

    WorkflowDefinition* definition = repository.find(definitionId);

    if (definition == nullptr) {

        throw std::out_of_range("Definition not found: " + definitionId);

    }
    if (!definition->isValidated()) {

        throw std::logic_error("Definition '" + definitionId + "' is a draft and cannot be started");
    }

    WorkflowInstance* instance = definition->createInstance(data);

    // Take what we still need BEFORE ownership moves to the engine.
    WorkflowInstance* running = instance.get();

    std::string instanceId = instance->getId();

    engine.registerInstance(std::move(instance));

    running->start();

    return instanceId;
}

std::string WorkflowFacade::getInstanceStatus(const std::string& instanceId) {

    WorkflowInstance* instance = engine.findInstance(instanceId);

    if (instance == nullptr) {
        
        throw std::out_of_range("Instance not found: " + instanceId);
    }
    return instance->getStatus();
}
