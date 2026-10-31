#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/States.h"
#include "tividy/core/Common.h"
#include "tividy/core/Participant.h" 
#include <stdexcept>

WorkItem::WorkItem(const std::string& id, const std::string& name)
    : id(id), name(name), assignee(nullptr), state(CreatedState::instance()) {}

WorkItem::~WorkItem() {
    releaseAssignee();
}



void WorkItem::start() {
    state->start(*this); 
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
        mediator->notify(this, WorkflowEvent(EVT_REJECTED, 0, id, 0, reason));
    }
    if (getParent()) getParent()->childFinished(*this);
}

void WorkItem::cancel() {
    state->cancel(*this); 
    releaseAssignee();
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_CANCELLED, 0, id));
    if (getParent()) getParent()->childFinished(*this);
}

void WorkItem::escalate(const std::string& reason) {
    state->escalate(*this); 
    failureReason = reason;
    releaseAssignee();      
    
    if (mediator) {
        mediator->notify(this, WorkflowEvent(EVT_ESCALATED, 0, id, 0, reason));
    }
}


void WorkItem::markEscalated() {
    releaseAssignee();
    setState(EscalatedState::instance());
}


void WorkItem::makeAvailable() { state->makeAvailable(*this); }
void WorkItem::assign()        { state->assign(*this); }

void WorkItem::adoptAssignee(Participant* p) {
    releaseAssignee();
    assignee = p;
    if (p) p->itemAssigned();
}

void WorkItem::assign(Participant* p) {
    if (assignee == p) return;

    Participant* prev = assignee;
    
    if (prev) prev->itemReleased();
    assignee = p;
    if (assignee) assignee->itemAssigned();

    try {
        state->assign(*this);
        if (assignee) data["_assignee"] = assignee->getId();   // saved in mementos so a rollback can restore the person
    } catch (...) {
        if (assignee) assignee->itemReleased();
        assignee = prev;
        if (prev) prev->itemAssigned();
        throw;
    }
}

void WorkItem::suspend() { state->suspend(*this); }
void WorkItem::resume()  { state->resume(*this); }



void WorkItem::activate() { 
    if (isPending()) makeAvailable(); 
}

bool WorkItem::isComplete() const { return state == CompletedState::instance() || state == SkippedState::instance(); }
bool WorkItem::isWaitingForPickup() const { return state == AssignedState::instance(); }
bool WorkItem::isInProgress() const { return state == InProgressState::instance(); }
WorkItemState::Disposition WorkItem::getDisposition() const { return state->disposition(); }
bool WorkItem::wasRejected() const { return state->isRejection(); }
bool WorkItem::isSkipped() const  { return state == SkippedState::instance(); }

void WorkItem::skip(const std::string& reason) {
    state->skip(*this);
    failureReason = reason;
    if (mediator) mediator->notify(this, WorkflowEvent(EVT_SKIPPED, 0, id, 0, reason));
    if (getParent()) getParent()->childFinished(*this);
}

void WorkItem::escalateTo(Participant* next) {
    state->escalate(*this);          // Assigned -> Escalated (throws if the item cannot be escalated now)
    releaseAssignee();
    if (next) assign(next);          // Escalated -> Assigned, now owned by the next level
}

void WorkItem::collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& out) {
    out.push_back(std::make_pair(this, static_cast<WorkflowComponent*>(this)));
}
bool WorkItem::isFinished() const { return state->isFinal(); }
bool WorkItem::isFailed() const   { return state == RejectedState::instance(); }

WorkflowComponent* WorkItem::clone() const {
    WorkItem* copy = new WorkItem(id, name);
    copy->role = role;
    copy->assignmentRule = assignmentRule;
    copy->data = data; 
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

//New Data Access Methods

std::string WorkItem::getData(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = data.find(key);
    return (it != data.end()) ? it->second : "";
}

void WorkItem::setData(const std::string& key, const std::string& value) {
    data[key] = value;
}
// Restore persisted state without firing workflow events or changing dependencies.
// The memento is a historical snapshot, not a new user command.
void WorkItem::restoreSnapshot(const std::string& stateName,
                               const std::map<std::string, std::string>& snapshotData,
                               const std::string& snapshotOutcome,
                               const std::string& snapshotFailureReason) {
    releaseAssignee();
    if (WorkItemState* target = WorkItemState::byName(stateName)) setState(target);
    else throw std::invalid_argument("Unknown work-item state in memento: " + stateName);
    data = snapshotData;
    outcome = snapshotOutcome;
    failureReason = snapshotFailureReason;
}
