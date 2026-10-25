#ifndef WORKFLOWDIRECTOR_H
#define WORKFLOWDIRECTOR_H

#include "tividy/builder/WorkflowBuilder.h"
#include "tividy/core/Common.h"
/**
 * @file WorkflowDirector.h
 * @brief Director: knows the ORDER of build steps for one process.
 *
 * The base class builds a tiny generic "review" process so it is usable and testable.
 * Each industry derives its own director (see scenarios/) and overrides construct();
 * the generic engine classes never change.
 */
class WorkflowDirector {
public:
    WorkflowDirector();
    virtual ~WorkflowDirector();

    /// Drive @p builder through the steps of the process (names it with setIdentity first).
    virtual void construct(WorkflowBuilder& builder);
};

#endif
