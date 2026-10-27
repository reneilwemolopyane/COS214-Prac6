/**
 * @file ObserverPattern.cpp
 * @brief Implementation of WorkflowSubject and WorkflowObserver.
 */
#include "tividy/observer/ObserverPattern.h"
#include <algorithm>

WorkflowSubject::~WorkflowSubject() {
    // Observers may outlive the subject: clear their pointer so their destructor does not detach from a dead object.
    for (std::size_t i = 0; i < observers.size(); ++i)
        observers[i]->subject = 0;
}

void WorkflowSubject::attach(WorkflowObserver* o) {
    if (o && std::find(observers.begin(), observers.end(), o) == observers.end())
        observers.push_back(o);
}

void WorkflowSubject::detach(WorkflowObserver* o) {
    observers.erase(std::remove(observers.begin(), observers.end(), o), observers.end());
}

void WorkflowSubject::notifyObservers(const WorkflowEvent& e) {
    // Work on a snapshot so update() may attach/detach observers safely.
    std::vector<WorkflowObserver*> snapshot = observers;
    for (std::size_t i = 0; i < snapshot.size(); ++i)
        // Skip observers that were detached by an earlier update() in this same notify.
        if (std::find(observers.begin(), observers.end(), snapshot[i]) != observers.end())
            snapshot[i]->update(e);
}

WorkflowObserver::WorkflowObserver(WorkflowSubject* s) : subject(s) {
    if (subject) subject->attach(this);
}

WorkflowObserver::~WorkflowObserver() {
    if (subject) subject->detach(this);
}