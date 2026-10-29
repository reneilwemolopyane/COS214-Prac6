#include "tividy/decorator/WorkItemDecorator.h"

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

void WorkItemDecorator::accept(WorkflowVisitor& v) {
    v.visitDecorator(*this);   // let visitors see this layer
    wrapped->accept(v);        // then reach the real WorkItem
}

void WorkItemDecorator::reject(const std::string& reason) { wrapped->reject(reason); }

bool WorkItemDecorator::isOverdue() const { return wrapped->isOverdue(); }

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