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
    wrapped->cancel(); 
}

bool WorkItemDecorator::isComplete() const { 
    return wrapped->isComplete(); 
}

std::string WorkItemDecorator::getName() const { 
    return wrapped->getName(); 
}

void WorkItemDecorator::accept(WorkflowVisitor& v) { 
    wrapped->accept(v); 
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