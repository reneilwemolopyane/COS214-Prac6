#pragma once
/**
 * @file CommandProcessor.h
 * @brief Command pattern Invoker.
 */
#include <vector>
#include "tividy/command/Command.h"

class WorkflowInstance;

/**
 * @brief Command pattern Invoker. One per WorkflowInstance.
 *
 * Runs commands, keeps them in a history, retries failed ones and
 * compensates (undoes) completed ones in reverse order. After every
 * attempt it reports the result to its instance, which pushes an event to
 * the observers. The processor owns every command given to run().
 */
class CommandProcessor {
public:
    /** @param instance Instance that receives the result events (not owned). */
    explicit CommandProcessor(WorkflowInstance* instance = 0) : instance(instance) {}

    /** @brief Deletes every command in the history. */
    ~CommandProcessor();

    /**
     * @brief Takes ownership of the command, records it and executes it.
     * @return true if the command ends up DONE (including after retries triggered by observers).
     */
    bool run(WorkflowCommand* command);

    /**
     * @brief Executes a FAILED command from the history again.
     * @return true if it is now DONE. False if the command is unknown or not FAILED.
     */
    bool retry(WorkflowCommand* command);

    /** @brief Undoes every reversible DONE command, newest first, and marks it UNDONE. Skips commands that need no compensation and escalates the ones that cannot be undone. */
    void compensate();

    /** @brief All commands run so far, oldest first. */
    const std::vector<WorkflowCommand*>& getHistory() const { return history; }

private:
    /** @brief One execute() attempt: updates status and attempts, then raises the result event. */
    bool attempt(WorkflowCommand* command);

    /** @brief Whether the command is in this processor's history. */
    bool owns(WorkflowCommand* command) const;

    WorkflowInstance* instance;               ///< Receives result events (not owned).
    std::vector<WorkflowCommand*> history;    ///< Owned commands, oldest first.

    CommandProcessor(const CommandProcessor&);              ///< Not copyable.
    CommandProcessor& operator=(const CommandProcessor&);   ///< Not assignable.
};