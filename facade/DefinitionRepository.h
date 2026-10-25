#ifndef DEFINITIONREPOSITORY_H
#define DEFINITIONREPOSITORY_H

#include <map>
#include <memory>
#include <string>
#include "WorkflowDefinition.h"

/**
 * @file DefinitionRepository.h
 * @brief Stores workflow definitions (templates) by id. Instances are NOT stored here.
 *
 * The repository owns what it stores. Saving a definition with an existing id
 * replaces the older one.
 */
class DefinitionRepository {
public:
    DefinitionRepository();

    /**
     * @brief Store a definition, taking ownership.
     * @throws std::invalid_argument if @p definition is null.
     */
    void save(WorkflowDefinition* definition);

    /// @return A non-owning pointer to the definition, or nullptr if unknown.
    WorkflowDefinition* find(const std::string& id) const;

private:
    std::map<std::string, WorkflowDefinition*> definitions;
};

#endif
