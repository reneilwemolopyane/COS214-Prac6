#include "NotificationDecorator.h"

NotificationDecorator::NotificationDecorator(WorkflowComponent* w, Sink s)
    : WorkItemDecorator(w), sink(s) {}

void NotificationDecorator::tell(const std::string& event) {
    if (sink) sink(event + ": " + wrapped->getName());
}

void NotificationDecorator::start()    { 
    wrapped->start();    tell("started"); 
}

void NotificationDecorator::complete() { 
    wrapped->complete(); tell("completed"); 
}

void NotificationDecorator::cancel()   { 
    wrapped->cancel();   tell("cancelled"); 
}

WorkflowComponent* NotificationDecorator::clone() const {
    return new NotificationDecorator(wrapped->clone(), sink);
}