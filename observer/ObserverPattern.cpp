/**
 * @file ObserverPattern.cpp
 * @brief Implementation of WorkflowSubject.
 */
#include "tividy/observer/ObserverPattern.h"
#include <ctime>

namespace {

/** @brief Current local time as "YYYY-MM-DD HH:MM:SS". */
std::string now() {
    std::time_t t = std::time(0);
    std::tm local;
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local);
    return buf;
}

} // namespace

void WorkflowSubject::attach(EventType type, WorkflowObserver* observer) {
    if (!observer) return;
    for (std::size_t i = 0; i < subscriptions.size(); ++i)
        if (subscriptions[i].observer == observer &&
            (subscriptions[i].allTypes || subscriptions[i].type == type))
            return;
    Subscription s = { observer, type, false };
    subscriptions.push_back(s);
}

void WorkflowSubject::attach(WorkflowObserver* observer) {
    if (!observer) return;
    detach(observer);   // an all-types subscription replaces any single-type ones
    Subscription s = { observer, EVT_CREATED, true };
    subscriptions.push_back(s);
}

void WorkflowSubject::detach(WorkflowObserver* observer) {
    for (std::size_t i = subscriptions.size(); i > 0; --i)
        if (subscriptions[i - 1].observer == observer)
            subscriptions.erase(subscriptions.begin() + (i - 1));
}

void WorkflowSubject::detach(EventType type, WorkflowObserver* observer) {
    for (std::size_t i = subscriptions.size(); i > 0; --i)
        if (subscriptions[i - 1].observer == observer && !subscriptions[i - 1].allTypes &&
            subscriptions[i - 1].type == type)
            subscriptions.erase(subscriptions.begin() + (i - 1));
}

void WorkflowSubject::notifyObservers(const WorkflowEvent& event) {
    WorkflowEvent e = event;
    if (e.timestamp.empty()) e.timestamp = now();

    // Collect first: update() may change the subscriptions while we iterate.
    std::vector<WorkflowObserver*> targets;
    for (std::size_t i = 0; i < subscriptions.size(); ++i)
        if (subscriptions[i].allTypes || subscriptions[i].type == e.type)
            targets.push_back(subscriptions[i].observer);

    for (std::size_t i = 0; i < targets.size(); ++i) {
        // Skip observers that detached themselves during this notification.
        bool stillAttached = false;
        for (std::size_t j = 0; j < subscriptions.size() && !stillAttached; ++j)
            stillAttached = subscriptions[j].observer == targets[i];
        if (stillAttached) targets[i]->update(e);
    }
}
