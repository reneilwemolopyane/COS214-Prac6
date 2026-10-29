#include "tividy/facade/DefinitionRepository.h"
#include <stdexcept>

DefinitionRepository::DefinitionRepository() {}

void DefinitionRepository::save(WorkflowDefinition* definition) {
    if (!definition) {
        throw std::invalid_argument("Cannot save a null definition");
    }

    const std::string id = definition->getId();

    std::map<std::string, WorkflowDefinition*>::iterator it =
        definitions.find(id);

    if (it != definitions.end()) {
        if (it->second != definition) {
            delete it->second;
            it->second = definition;
        }
    } else {
        definitions[id] = definition;
    }
}

WorkflowDefinition* DefinitionRepository::find(const std::string& id) const {

    std::map<std::string, WorkflowDefinition* >::const_iterator it = definitions.find(id);

    return it == definitions.end() ? nullptr : it->second;
}

DefinitionRepository::~DefinitionRepository() {
    for (std::map<std::string, WorkflowDefinition*>::iterator it =
             definitions.begin();
         it != definitions.end(); ++it) {
        delete it->second;
    }
}