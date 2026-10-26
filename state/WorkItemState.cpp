#include "tividy/state/WorkItemState.h"
#include "tividy/state/InvalidTransitionException.h"
#include "tividy/composite_prototype/WorkItem.h"

void WorkItemState::makeAvailable(WorkItem& i) { throw InvalidTransitionException("makeAvailable", name(), i.getName()); }
void WorkItemState::assign(WorkItem& i)        { throw InvalidTransitionException("assign",        name(), i.getName()); }
void WorkItemState::start(WorkItem& i)         { throw InvalidTransitionException("start",         name(), i.getName()); }
void WorkItemState::complete(WorkItem& i)      { throw InvalidTransitionException("complete",      name(), i.getName()); }
void WorkItemState::reject(WorkItem& i)        { throw InvalidTransitionException("reject",        name(), i.getName()); }
void WorkItemState::cancel(WorkItem& i)        { throw InvalidTransitionException("cancel",        name(), i.getName()); }
void WorkItemState::escalate(WorkItem& i)      { throw InvalidTransitionException("escalate",      name(), i.getName()); }
void WorkItemState::suspend(WorkItem& i)       { throw InvalidTransitionException("suspend",       name(), i.getName()); }
void WorkItemState::resume(WorkItem& i)        { throw InvalidTransitionException("resume",        name(), i.getName()); }