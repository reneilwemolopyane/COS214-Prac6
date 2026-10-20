#pragma once
#include <map>

#include "../builder/WorkflowDefinition.h"


/** @brief Stores definitions. STUB */
class DefinitionRepository {
public:

    unique_ptr<WorkflowDefinition*>  add(WorkflowDefinition*);

    WorkflowDefinition* find(const std::string&);// TODO



private:

std::map<std::string, WorkflowDefinition*> defs;
};
