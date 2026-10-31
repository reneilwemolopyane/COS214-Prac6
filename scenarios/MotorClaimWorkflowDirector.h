#ifndef MOTORCLAIMWORKFLOWDIRECTOR_H
#define MOTORCLAIMWORKFLOWDIRECTOR_H

#include "tividy/builder/WorkflowDirector.h"

/**
 * @brief Builds the motor-vehicle insurance claim process used to demonstrate TIVIDY.
 *
 * This is scenario configuration, not workflow-engine logic. The definition describes
 * a normal repair-and-settlement path. The coordinator's existing rejection and
 * escalation behaviour handles exceptional paths at runtime.
 */
class MotorClaimWorkflowDirector : public WorkflowDirector {
public:
    void construct(WorkflowBuilder& builder) override;
};

#endif
