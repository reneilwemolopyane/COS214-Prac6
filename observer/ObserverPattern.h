#pragma once
/**
 * @file ObserverPattern.h
 * @brief Observer pattern (push model): WorkflowSubject and WorkflowObserver.
 *
 * The subject pushes each WorkflowEvent to the observers that subscribed to its
 * type. Observers never hold a pointer back to the subject, so an observer that
 * is destroyed first cannot leave a dangling pointer behind, and the subject never
 * owns its observers.
 */
#include <cstddef>
#include <vector>
#include "tividy/core/WorkflowEvent.h"

/**
 * @brief Observer: reacts to the workflow events it subscribed to.
 */
class WorkflowObserver {
public:
    virtual ~WorkflowObserver() {}

    /**
     * @brief Called by the subject for every subscribed event.
     * @param event The event (valid only for the duration of the call; copy what you keep).
     */
    virtual void update(const WorkflowEvent& event) = 0;
};

/**
 * @brief Subject: keeps the subscriptions and pushes events to them.
 *
 * WorkflowInstance is the concrete subject in the running system.
 * Observers are not owned by the subject: whoever attaches an observer must
 * keep it alive until it is detached or the subject is destroyed.
 */
class WorkflowSubject {
public:
    virtual ~WorkflowSubject() {}

    /** @brief Subscribes @p observer to one event type. Duplicates are ignored. */
    void attach(EventType type, WorkflowObserver* observer);

    /** @brief Subscribes @p observer to every event type. Duplicates are ignored. */
    void attach(WorkflowObserver* observer);

    /** @brief Removes every subscription of @p observer. */
    void detach(WorkflowObserver* observer);

    /** @brief Removes @p observer's subscription to one event type. */
    void detach(EventType type, WorkflowObserver* observer);

    /**
     * @brief Pushes @p event to every observer subscribed to its type.
     *
     * Fills in the timestamp if it is empty. Notifies from a copy of the
     * subscription list, so an observer may attach, detach or trigger new
     * events (e.g. retry a command) from inside update().
     * Subclasses may override this to complete the event before it is pushed.
     */
    virtual void notifyObservers(const WorkflowEvent& event);

    /** @brief Number of subscriptions (an observer on two types counts twice). */
    std::size_t subscriptionCount() const { return subscriptions.size(); }

private:
    struct Subscription {
        WorkflowObserver* observer;
        EventType type;
        bool allTypes;
    };
    std::vector<Subscription> subscriptions;
};
