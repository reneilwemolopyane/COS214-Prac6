#ifndef WORKFLOWDEFINITION_H
#define WORKFLOWDEFINITION_H

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Common.h"
#include "WorkComponent.h"
#include "WorkflowInstance.h"

/**
 * @file WorkflowDefinition.h
 * @brief The reusable template of a process.
 *
 * GoF roles: Product (Builder) and Client (Prototype).
 *
 * A definition owns a template tree of WorkComponent objects. It never runs;
 * createInstance() deep-copies the template so every WorkflowInstance owns its
 * own independent structure.
 */
class WorkflowDefinition {
public:
    /**
     * @brief Create an empty, not-yet-validated definition.
     * @param id      Unique identifier, e.g. "motor-claim".
     * @param version Definition version number.
     */
    WorkflowDefinition(const std::string& id, int version);

    /**
     * @brief Create a new, independent instance by cloning the template (Prototype).
     * @param data Data payload for the new instance.
     * @return The new instance; the caller owns it.
     * @throws std::logic_error if the definition has no structure yet.
     */
    WorkflowInstance* createInstance(const DataMap& data);

    /// @return The definition id.
    std::string getId() const;
    /// @return The definition version.
    int getVersion() const;
    /// @return true only if a validating builder produced this definition.
    bool isValidated() const;

    /// Take ownership of the template root (called by builders).
    void setRoot(WorkComponent* root);
    /// Mark the definition as validated (called by the validating builder).
    void markValidated();

    /// @return The template root (read only), or nullptr if none.
    const WorkComponent* getRoot() const;

    /// Record that @p to may only start after @p from has finished.
    void addDependency(const std::string& from, const std::string& to);
    /// @return All recorded (from, to) dependencies.
    const std::vector<std::pair<std::string, std::string> >& getDependencies() const;

    /// Attach a routing/approval rule.
    void addRule(const DataMap& rule);
    /// @return All attached rules.
    const std::vector<Rule>& getRules() const;

private:

    std::string id;

    int version;

    bool validated;

    WorkComponent* root;

    std::vector<std::pair<std::string, std::string> > dependencies;

    DataMap rules;

    int instanceCounter;
};

#endif