#ifndef DRAFTWORKFLOWBUILDER_H
#define DRAFTWORKFLOWBUILDER_H

#include "WorkflowBuilder.h"

/**
 * @file DraftWorkflowBuilder.h
 * @brief ConcreteBuilder that accepts incomplete input.
 *
 * Lets someone save a half-finished process. The result is never marked
 * validated, so the Facade refuses to start it.
 */

class DraftWorkflowBuilder : public WorkflowBuilder {

public:
    DraftWorkflowBuilder();

    void buildStage(const std::string& name) override;

    void buildWorkItem(const std::string& name, const std::string& role) override;

    void buildDependency(const std::string& from, const std::string& to) override;

    void buildRule(const DataMap& rule) override;

    /// Returns the (unvalidated) draft; never throws for incomplete input.
    WorkflowDefinition* getResult() override;
};

#endif
