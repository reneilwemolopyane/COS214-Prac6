#ifndef VALIDATINGWORKFLOWBUILDER_H
#define VALIDATINGWORKFLOWBUILDER_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "tividy/builder/WorkflowBuilder.h"

/**
 * @file ValidatingWorkflowBuilder.h
 * @brief ConcreteBuilder that refuses to produce a broken definition.
 *
 * Checks while building (unknown/duplicate names, order of calls) and when
 * getResult() is called (every item has a role, no circular dependencies).
 * A successful result is marked validated, so it may be started.
 */
class ValidatingWorkflowBuilder : public WorkflowBuilder {
public:
    ValidatingWorkflowBuilder();

    void reset() override;

    /// @throws std::invalid_argument for an empty or duplicate stage name.
    void buildStage(const std::string& name) override;

    /// @throws std::logic_error if no stage exists yet; std::invalid_argument for an empty or duplicate name.
    void buildWorkItem(const std::string& name, const std::string& role) override;

    /// @throws std::invalid_argument if either item is unknown or an item depends on itself.
    void buildDependency(const std::string& from, const std::string& to) override;

    void buildRule(const std::string& name, const std::string& value) override;

    /// @throws std::runtime_error if there are no items, an item has no role, or dependencies form a cycle.
    WorkflowDefinition* getResult() override;

    /// @return true if a work item with this name has been added.
    bool exists(const std::string& name) const;

    /// @return true if every work item has a non-empty role.
    bool allItemsHaveRoles() const;

    /// @return true if the recorded dependencies contain a cycle.
    bool hasCircularDependency() const;

private:
    std::map<std::string, std::string> itemRoles;

    std::vector<std::pair<std::string, std::string> > dependencies;

    std::set<std::string> stageNames;
};

#endif
