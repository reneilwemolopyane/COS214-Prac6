#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/States.h"
#include <stdexcept>

WorkItem::WorkItem(const std::string& n)
    : name(n), state(CreatedState::instance()) {}

void WorkItem::setState(WorkItemState* s) {
    if (s == nullptr) throw std::invalid_argument("WorkItem::setState(nullptr)");
    state = s;
}

const WorkItemState* WorkItem::getState() const { return state; }

void WorkItem::makeAvailable() { state->makeAvailable(*this); }
void WorkItem::assign()        { state->assign(*this); }
void WorkItem::start()         { state->start(*this); }
void WorkItem::complete()      { state->complete(*this); }
void WorkItem::reject()        { state->reject(*this); }
void WorkItem::cancel()        { state->cancel(*this); }
void WorkItem::escalate()      { state->escalate(*this); }
void WorkItem::suspend()       { state->suspend(*this); }
void WorkItem::resume()        { state->resume(*this); }

bool WorkItem::isComplete() const {
    return state != nullptr && state == CompletedState::instance();
}

WorkflowComponent* WorkItem::clone() const {
    // ctor sets CreatedState; only definition data (name) is copied.
    return new WorkItem(name);
}

std::string WorkItem::getName() const { return name; }

void WorkItem::accept(WorkflowVisitor&) {
    // Deferred: Visitor.h not provided. Leaf visit method name unknown.
}