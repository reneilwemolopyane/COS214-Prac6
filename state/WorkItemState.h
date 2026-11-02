#pragma once
#include <string>
#include "tividy/state/InvalidTransitionException.h"

class WorkItem;

/**
 * @brief State interface defining the legal operations and status of a work item.
 *
 * Concrete states override the operations they allow; unsupported transitions
 * throw InvalidTransitionException. State instances are shared by all items.
 */
class WorkItemState {
public:
    /** @brief Virtual destructor for derived work-item states. */
    virtual ~WorkItemState() {}

    /** @return The display and persistence name of this state. */
    virtual std::string name() const = 0;

    /** @brief Makes the item available for assignment. */
    virtual void makeAvailable(WorkItem& i);
    /** @brief Assigns the item to a participant. */
    virtual void assign(WorkItem& i);
    /** @brief Starts work on the item. */
    virtual void start(WorkItem& i);
    /** @brief Marks the item complete. */
    virtual void complete(WorkItem& i);
    /** @brief Rejects the item. */
    virtual void reject(WorkItem& i);
    /** @brief Cancels the item. */
    virtual void cancel(WorkItem& i);
    /** @brief Escalates the item. */
    virtual void escalate(WorkItem& i);
    /** @brief Suspends work on the item. */
    virtual void suspend(WorkItem& i);
    /** @brief Resumes suspended work. */
    virtual void resume(WorkItem& i);
    /** @brief Branch not taken: Created -> Skipped (dead-path elimination). */
    virtual void skip(WorkItem& i);

    /**
     * @brief High-level outcome category used by workflow coordination.
     */
    enum Disposition { OPEN, DONE, NOT_NEEDED, BLOCKED };
    /** @return This state's outcome category; defaults to OPEN. */
    virtual Disposition disposition() const { return OPEN; }
    /** @return Whether this state represents rejection. */
    virtual bool isRejection() const { return false; }
    /**
     * @brief Finds a shared state object by its persisted name.
     * @param stateName Name to look up.
     * @return The shared state object, or nullptr if the name is unknown.
     */
    static WorkItemState* byName(const std::string& stateName);

    /** @return Whether this state is terminal. */
    virtual bool isFinal() const { return false; }
    /** @return Whether this state is the initial Created state. */
    virtual bool isCreated() const { return false; }
    /** @return Whether this state is Available. */
    virtual bool isAvailable() const { return false; }
};