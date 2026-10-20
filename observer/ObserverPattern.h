#pragma once
/** @file ObserverPattern.h  Observer: WorkflowSubject + WorkflowObserver. STUB */
#include <vector>
#include "tividy/core/WorkflowEvent.h"

class WorkflowObserver;

class WorkflowSubject {
public:
    virtual ~WorkflowSubject() {}
    void attach(WorkflowObserver*) {}                 // TODO
    void detach(WorkflowObserver*) {}                 // TODO
    void notify() {}                                  // TODO
    void setState(const WorkflowEvent&) {}            // TODO
    const WorkflowEvent& getState() const { return state; }
protected:
    std::vector<WorkflowObserver*> observers;
    WorkflowEvent state;
};

class WorkflowObserver {
public:
    explicit WorkflowObserver(WorkflowSubject* s) : subject(s) {}   // TODO: attach
    virtual ~WorkflowObserver() {}                                   // TODO: detach
    virtual void update() = 0;
protected:
    WorkflowSubject* subject;
};
