#pragma once
/** @file States.h  ConcreteStates. STUB: transitions TODO (see Task 6 state diagram). */
#include "tividy/state/WorkItemState.h"
#define TIVIDY_STATE(C, N) class C : public WorkItemState { public: std::string name() const { return N; } /* TODO transitions */ };
TIVIDY_STATE(CreatedState,   "Created")
TIVIDY_STATE(AvailableState, "Available")
TIVIDY_STATE(AssignedState,  "Assigned")
TIVIDY_STATE(InProgressState,"InProgress")
TIVIDY_STATE(SuspendedState, "Suspended")
TIVIDY_STATE(EscalatedState, "Escalated")
TIVIDY_STATE(CompletedState, "Completed")   // final
TIVIDY_STATE(RejectedState,  "Rejected")    // final
TIVIDY_STATE(CancelledState, "Cancelled")   // final
#undef TIVIDY_STATE
