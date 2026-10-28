#include "tividy/facade/WorkflowFacade.h"
#include <stdexcept>

WorkflowFacade::WorkflowFacade(DefinitionRepository& repository, WorkflowEngine& engine)
    : repository(repository), engine(engine) {}

void WorkflowFacade::registerDefinition(WorkflowDefinition* definition) {

    repository.save(definition);
}

std::string WorkflowFacade::startWorkflow(const std::string& definitionId,const DataMap& data) {

    WorkflowDefinition* definition = repository.find(definitionId);

    if (definition == nullptr)
        throw std::out_of_range("Definition not found: " + definitionId);

    if (!definition->isValidated())
        throw std::logic_error("Definition is not validated: " + definitionId);

    WorkflowInstance* instance = definition->createInstance(data);
    std::string instanceId = instance->getId();

    if (!engine.registerInstance(instance)) {
        delete instance;
        throw std::logic_error("Could not register workflow instance: " + instanceId);
    }

    if (!engine.start(instance)) {
        // The engine owns the instance now, so do not delete it here.
        throw std::logic_error("Could not start workflow instance: " + instanceId);
    }

    return instanceId;
}

std::string WorkflowFacade::getInstanceStatus(
    const std::string& instanceId
) {
    return engine.getStatus(instanceId);
}