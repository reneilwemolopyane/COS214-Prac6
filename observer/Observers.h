#pragma once
/** @file Observers.h  ConcreteObservers. STUB: update() bodies are TODO. */
#include "tividy/observer/ObserverPattern.h"
class CommandProcessor;

class DependencyTracker : public WorkflowObserver {   // on COMPLETED release next items
public:
    explicit DependencyTracker(WorkflowSubject* s) : WorkflowObserver(s) {}
    void update() {}  // TODO
};
class AuditLogger : public WorkflowObserver {          // record every event
public:
    explicit AuditLogger(WorkflowSubject* s) : WorkflowObserver(s) {}
    void update() {}  // TODO
    std::vector<WorkflowEvent> trail;
};
class NotificationService : public WorkflowObserver {  // notify next participant
public:
    explicit NotificationService(WorkflowSubject* s) : WorkflowObserver(s) {}
    void update() {}  // TODO
};
class IncidentMonitor : public WorkflowObserver {      // on FAILED retry then incident
public:
    IncidentMonitor(WorkflowSubject* s, CommandProcessor* p) : WorkflowObserver(s), processor(p) {}
    void update() {}  // TODO
private:
    CommandProcessor* processor;
};
