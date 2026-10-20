#pragma once
#include <map>
#include "tividy/builder/WorkflowDefinition.h"
/** @brief Stores definitions. STUB */
class DefinitionRepository {
public:
    void add(WorkflowDefinition*) {}                        // TODO
    WorkflowDefinition* find(const std::string&) { return 0; }  // TODO
private: std::map<std::string, WorkflowDefinition*> defs;
};
