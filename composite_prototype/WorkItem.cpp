#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/States.h"
#include "tividy/core/Common.h"
#include <stdexcept>

WorkItem::WorkItem(const std::string& n)
    : id(n), name(n), assignee(nullptr), state(CreatedState::instance()) {}

WorkItem::~WorkItem() {}


void WorkItem::start() {
    state->start(*this); 
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_STARTED, id));
}

void WorkItem::complete() {
    state->complete(*this); 
    releaseAssignee();
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_COMPLETED, id));
    if (getParent()) getParent()->childFinished(*this); // Push model
}

void WorkItem::reject(const std::string& reason) {
    state->reject(*this); 
    failureReason = reason;
    releaseAssignee();
    DataMap d; d["reason"] = reason;
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_REJECTED, id, d));
    if (getParent()) getParent()->childFinished(*this); // Push model
}

void WorkItem::cancel() {
    state->cancel(*this); 
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_CANCELLED, id));
    if (getParent()) getParent()->childFinished(*this); // Push model
}

void WorkItem::escalate(const std::string& reason) {
    state->escalate(*this); 
    DataMap d; d["reason"] = reason;
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_ESCALATED, id, d));
}

void WorkItem::makeAvailable() { state->makeAvailable(*this); }
void WorkItem::assign()        { state->assign(*this); }

void WorkItem::assign(Participant* p) {
    Participant* prev = assignee;
    assignee = p;
    try {
        state->assign(*this);
    } catch (...) {
        assignee = prev; // Rollback on failure
        throw;
    }
}

void WorkItem::suspend() { state->suspend(*this); }
void WorkItem::resume()  { state->resume(*this); }

void WorkItem::activate() { makeAvailable(); }

bool WorkItem::isComplete() const { return state == CompletedState::instance(); }
bool WorkItem::isFinished() const { return state->isFinal(); }
bool WorkItem::isFailed() const   { return state == RejectedState::instance(); }

WorkflowComponent* WorkItem::clone() const {
    WorkItem* copy = new WorkItem(name);
    copy->id = id;
    copy->role = role;
    return copy;
}

std::string WorkItem::getName() const { return name; }

void WorkItem::accept(WorkflowVisitor& v) {
    v.visitWorkItem(*this);
}

bool WorkItem::isPending() const   { return state->isCreated(); }
bool WorkItem::isAvailable() const { return state->isAvailable(); }
void WorkItem::releaseAssignee()   { assignee = nullptr; }
void WorkItem::setState(WorkItemState* s) { if (s) state = s; }
const WorkItemState* WorkItem::getState() const { return state; }
std::string WorkItem::getStateName() const { return state->name(); }