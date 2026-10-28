#ifndef WORKFLOWBUILDER_H
#define WORKFLOWBUILDER_H

#include <memory>
#include <string>
#include "tividy/core/Common.h"
#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/composite_prototype/Stage.h"

class Stage;
class WorkflowDefinition;

/**
 * @file WorkflowBuilder.h
 * @brief Abstract Builder: assembles a WorkflowDefinition step by step.
 *
 * GoF role: Builder. ValidatingWorkflowBuilder and DraftWorkflowBuilder are the
 * ConcreteBuilders, WorkflowDirector is the Director and WorkflowDefinition the Product.
 *
 * After getResult() the builder resets itself, so one builder can build many definitions.
 */
class WorkflowBuilder {
public:
    WorkflowBuilder();

    virtual ~WorkflowBuilder();

    /**
     * @brief Set the id and version of the definition being built. Also resets the builder.
     * (Small addition to the diagram: the Director needs a way to name the process.)
     */
    virtual void setIdentity(const std::string& id, int version);

    /// Discard any partly built product and start a fresh, empty one.
    virtual void reset();

    /// Add a stage; later work items are added inside the most recent stage.
    virtual void buildStage(const std::string& name) = 0;

    /// Add a work item to the current stage.
    virtual void buildWorkItem(const std::string& name, const std::string& role) = 0;

    /// Record that @p to depends on @p from.
    virtual void buildDependency(const std::string& from, const std::string& to) = 0;

    /// Attach a rule to the definition.
    virtual void buildRule(const std::string& name, const std::string& value) = 0;

    /// Hand over the finished definition (and reset the builder).
    virtual WorkflowDefinition* getResult() = 0;

protected:
    /// Add a stage under the root and make it current.
    void appendStage(const std::string& name);

    /// Add a work item to the current stage (a "Default" stage is created if none exists).
    void appendItem(const std::string& name, const std::string& role);
    
    /// Move the product out and reset the builder for the next build.
    WorkflowDefinition* release();

    WorkflowDefinition* product;  ///< Definition being built.

    Stage* rootStage;            ///< Non-owning: root inside the product.

    Stage* currentStage;         ///< Non-owning: stage receiving new items.

    std::string id;

    int version;
};

#endif

