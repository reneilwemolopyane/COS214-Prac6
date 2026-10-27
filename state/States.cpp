#include "tividy/state/States.h"
#include "tividy/composite_prototype/WorkItem.h"

// ---- CreatedState ----
std::string CreatedState::name() const { return "Created"; }
WorkItemState* CreatedState::instance() { static CreatedState s; return &s; }
void CreatedState::makeAvailable(WorkItem& i) { i.setState(AvailableState::instance()); }
void CreatedState::cancel(WorkItem& i)        { i.setState(CancelledState::instance()); }

// ---- AvailableState ----
std::string AvailableState::name() const { return "Available"; }
WorkItemState* AvailableState::instance() { static AvailableState s; return &s; }
void AvailableState::assign(WorkItem& i) { i.setState(AssignedState::instance()); }
void AvailableState::cancel(WorkItem& i) { i.setState(CancelledState::instance()); }

// ---- AssignedState ----
std::string AssignedState::name() const { return "Assigned"; }
WorkItemState* AssignedState::instance() { static AssignedState s; return &s; }
void AssignedState::assign(WorkItem& i)   { i.setState(AssignedState::instance()); }   // amendment
void AssignedState::start(WorkItem& i)    { i.setState(InProgressState::instance()); }
void AssignedState::cancel(WorkItem& i)   { i.setState(CancelledState::instance()); }
void AssignedState::escalate(WorkItem& i) { i.setState(EscalatedState::instance()); }

// ---- EscalatedState ----
std::string EscalatedState::name() const { return "Escalated"; }
WorkItemState* EscalatedState::instance() { static EscalatedState s; return &s; }
void EscalatedState::start(WorkItem& i)    { i.setState(InProgressState::instance()); }
void EscalatedState::complete(WorkItem& i) { i.setState(CompletedState::instance()); }
void EscalatedState::cancel(WorkItem& i)   { i.setState(CancelledState::instance()); }

// ---- InProgressState ----
std::string InProgressState::name() const { return "InProgress"; }
WorkItemState* InProgressState::instance() { static InProgressState s; return &s; }
void InProgressState::complete(WorkItem& i) { i.setState(CompletedState::instance()); }
void InProgressState::reject(WorkItem& i)   { i.setState(RejectedState::instance()); }
void InProgressState::cancel(WorkItem& i)   { i.setState(CancelledState::instance()); }
void InProgressState::suspend(WorkItem& i)  { i.setState(SuspendedState::instance()); }

// ---- SuspendedState ----
std::string SuspendedState::name() const { return "Suspended"; }
WorkItemState* SuspendedState::instance() { static SuspendedState s; return &s; }
void SuspendedState::cancel(WorkItem& i) { i.setState(CancelledState::instance()); }   // amendment
void SuspendedState::resume(WorkItem& i) { i.setState(InProgressState::instance()); }

// ---- Final states: all operations inherited from base -> throw ----
std::string CompletedState::name() const { return "Completed"; }
WorkItemState* CompletedState::instance() { static CompletedState s; return &s; }
void CompletedState::reopen(WorkItem& i) { i.setState(InProgressState::instance()); }
std::string RejectedState::name() const { return "Rejected"; }
WorkItemState* RejectedState::instance() { static RejectedState s; return &s; }
std::string CancelledState::name() const { return "Cancelled"; }
WorkItemState* CancelledState::instance() { static CancelledState s; return &s; }