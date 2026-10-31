#ifndef EDUCATIONBURSARYWORKFLOWDIRECTOR_H
#define EDUCATIONBURSARYWORKFLOWDIRECTOR_H

#include "tividy/builder/WorkflowDirector.h"

/**
 * @brief Builds the university bursary application used as TIVIDY's second industry (education).
 *
 * Pure configuration: it names work items, roles, dependencies and the text of routing rules.
 * It reuses the same Builder, engine, approval chain, decorators and adapters as insurance.
 */
class EducationBursaryWorkflowDirector : public WorkflowDirector {
public:
    void construct(WorkflowBuilder& builder) override;
};

#endif
