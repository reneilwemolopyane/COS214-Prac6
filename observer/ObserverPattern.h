#pragma once
/**
 * @file ObserverPattern.h
 * @brief Observer pattern: abstract Subject and Observer (push model).
 *
 * The subject pushes a WorkflowEvent to every attached observer through
 * update(e). Observers never call back into the subject to fetch state.
 */
#include <vector>
#include "core/WorkflowEvent.h"

class WorkflowObserver;

/**
 * @brief Observer pattern Subject.
 *
 * Keeps the list of attached observers and pushes events to them.
 * WorkflowInstance is the ConcreteSubject.
 */
class WorkflowSubject {
public:
    WorkflowSubject() {}

    /** @brief Tells every still-attached observer that this subject is gone, so none keeps a dangling pointer. */
    virtual ~WorkflowSubject();

    /** @brief Adds an observer. Ignores null and duplicates. */
    void attach(WorkflowObserver* o);

    /** @brief Removes an observer. Safe to call even if it was never attached. */
    void detach(WorkflowObserver* o);

    /**
     * @brief Pushes the event to every attached observer (push model).
     *
     * Iterates over a copy of the list, so an observer may detach itself
     * (or another observer) during update() without breaking the loop.
     */
    void notifyObservers(const WorkflowEvent& e);

    /** @brief Number of observers currently attached. */
    std::size_t observerCount() const { return observers.size(); }

protected:
    std::vector<WorkflowObserver*> observers;   ///< Attached observers (not owned).

private:
    WorkflowSubject(const WorkflowSubject&);              ///< Not copyable.
    WorkflowSubject& operator=(const WorkflowSubject&);   ///< Not assignable.
};

/**
 * @brief Observer pattern Observer.
 *
 * Attaches itself to its subject on construction and detaches on
 * destruction. Concrete observers implement update(e) and react only to
 * the event types they care about.
 */
class WorkflowObserver {
    friend class WorkflowSubject;   ///< Lets the subject clear @ref subject when it is destroyed first.
public:
    /** @brief Stores the subject and attaches to it. */
    explicit WorkflowObserver(WorkflowSubject* s);

    /** @brief Detaches from the subject if it still exists. */
    virtual ~WorkflowObserver();

    /** @brief Receives a pushed event from the subject. */
    virtual void update(const WorkflowEvent& e) = 0;

    /** @brief The observed subject, or null if it has been destroyed. */
    WorkflowSubject* getSubject() const { return subject; }

protected:
    WorkflowSubject* subject;   ///< Observed subject (not owned).

private:
    WorkflowObserver(const WorkflowObserver&);              ///< Not copyable.
    WorkflowObserver& operator=(const WorkflowObserver&);   ///< Not assignable.
};
