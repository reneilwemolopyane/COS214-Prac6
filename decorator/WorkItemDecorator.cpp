#include "WorkItemDecorator.h"

WorkItemDecorator::WorkItemDecorator(WorkflowComponent* w) : wrapped(w) {
    if (wrapped) wrapped->setParent(this);   // wrapped item reports to the decorator
}

WorkItemDecorator::~WorkItemDecorator() {
    delete wrapped;
}

void WorkItemDecorator::start()    { 
    wrapped->start(); 
}

void WorkItemDecorator::complete() { 
    wrapped->complete(); 
}

void WorkItemDecorator::cancel()   { 
    wrapped->cancel(); }

bool WorkItemDecorator::isComplete() const { 
    return wrapped->isComplete(); 
}
std::string WorkItemDecorator::getName() const { 
    return wrapped->getName(); 
}

long WorkItemDecorator::adjustDeadline(long baseSeconds) const { return wrapped ? wrapped->adjustDeadline(baseSeconds) : baseSeconds; }

WorkItem* WorkItemDecorator::underlyingItem() { return wrapped ? wrapped->underlyingItem() : 0; }

void WorkItemDecorator::collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& out) {
    if (WorkItem* item = underlyingItem()) out.push_back(std::make_pair(item, static_cast<WorkflowComponent*>(this)));
}

void WorkItemDecorator::accept(WorkflowVisitor& v) { 
    wrapped->accept(v); 
}

void WorkItemDecorator::assign(Participant* p) { 
    wrapped->assign(p); 
}

void WorkItemDecorator::complete(const std::string& outcome) { 
    wrapped->complete(outcome); 
}

void WorkItemDecorator::activate()         { 
    wrapped->activate(); 
}

bool WorkItemDecorator::isFinished() const { 
    return wrapped->isFinished(); 
}

bool WorkItemDecorator::isFailed() const   { 
    return wrapped->isFailed(); 
}

void WorkItemDecorator::childFinished(WorkflowComponent&) {
    if (getParent()) getParent()->childFinished(*this);   // report as the decorated item
}