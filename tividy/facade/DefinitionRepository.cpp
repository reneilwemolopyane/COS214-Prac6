#include "tividy/facade/DefinitionRepository.h"
#include <stdexcept>

DefinitionRepository::DefinitionRepository() {}

void DefinitionRepository::save(WorkflowDefinition* definition) {

    if (!definition) {

        throw std::invalid_argument("Cannot save a null definition");
    }

    std::string id = definition->getId();

    definitions[id] = definition;
}

WorkflowDefinition* DefinitionRepository::find(const std::string& id) const {

    std::map<std::string, WorkflowDefinition* >::const_iterator it = definitions.find(id);

    return it == definitions.end() ? nullptr : it->second;
}
