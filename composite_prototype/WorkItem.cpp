#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/States.h"
#include "tividy/core/Common.h"
#include <stdexcept>

WorkItem::WorkItem(const std::string& id, const std::string& name)
    : id(id), name(name), assignee(nullptr), state(CreatedState::instance()) {}

WorkItem::~WorkItem() {
    releaseAssignee();
}



void WorkItem::start() {
    state->start(*this); 
    // Fixed: Using EVT_ prefix from Common.h
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_STARTED, 0, id));
}

void WorkItem::complete() {
    state->complete(*this); 
    releaseAssignee();
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_COMPLETED, 0, id));
    if (getParent()) getParent()->childFinished(*this);
}

void WorkItem::reject(const std::string& reason) {
    state->reject(*this); 
    failureReason = reason;
    releaseAssignee();
    
    if (mediator) {
        // Fixed: Using WorkflowEvent struct fields (details) instead of DataMap
        mediator->notify(this, WorkflowEvent(EVT_REJECTED, 0, id, 0, reason));
    }
    if (getParent()) getParent()->childFinished(*this);
}

void WorkItem::cancel() {
    state->cancel(*this); 
    releaseAssignee(); // Fixed: Release before notifying
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_CANCELLED, 0, id));
    if (getParent()) getParent()->childFinished(*this);
}

void WorkItem::escalate(const std::string& reason) {
    state->escalate(*this); 
    failureReason = reason; // Fixed: Set failureReason
    releaseAssignee();      // Fixed: Release assignee
    
    if (mediator) {
        mediator->notify(this, WorkflowEvent(EVT_ESCALATED, 0, id, 0, reason));
    }
}



void WorkItem::makeAvailable() { state->makeAvailable(*this); }
void WorkItem::assign()        { state->assign(*this); }

void WorkItem::assign(Participant* p) {
    if (assignee == p) return;

    Participant* prev = assignee;
    
    if (prev) prev->itemReleased();
    assignee = p;
    if (assignee) assignee->itemAssigned();

    try {
        state->assign(*this);
    } catch (...) {
        
        if (assignee) assignee->itemReleased();
        assignee = prev;
        if (prev) prev->itemAssigned();
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
    WorkItem* copy = new WorkItem(id, name);
    copy->role = role;
    return copy;
}

std::string WorkItem::getName() const { return name; }

void WorkItem::accept(WorkflowVisitor& v) {
    v.visitWorkItem(*this);
}

bool WorkItem::isPending() const   { return state->isCreated(); }
bool WorkItem::isAvailable() const { return state->isAvailable(); }


void WorkItem::releaseAssignee() { 
    if (assignee) {
        assignee->itemReleased();
        assignee = nullptr; 
    }
}

void WorkItem::setState(WorkItemState* s) { if (s) state = s; }
const WorkItemState* WorkItem::getState() const { return state; }
std::string WorkItem::getStateName() const { return state->name(); }