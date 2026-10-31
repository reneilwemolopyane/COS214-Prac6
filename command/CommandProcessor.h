#pragma once
/**
 * @file CommandProcessor.h
 * @brief Command pattern Invoker: runs, retries and compensates workflow commands.
 */
#include <cstddef>
#include <vector>
#include "tividy/command/Command.h"

class WorkflowInstance;

/**
 * @brief Command pattern Invoker. One per WorkflowInstance.
 *
 * Runs commands, keeps them in a history, retries failed ones and undoes
 * completed ones in reverse order (compensation). Every result is reported
 * to its WorkflowInstance as an event.
 * The processor owns every command it has been given.
 */
class CommandProcessor {
public:
    /** @param inst Instance that receives the events (not owned, may be null). */
    explicit CommandProcessor(WorkflowInstance* inst = 0) : instance(inst) {}

    /** @brief Deletes every command in the history. */
    ~CommandProcessor();

    /**
     * @brief Takes ownership of a command and executes it.
     * @return true if the command ends in status DONE (including after retries).
     */
    bool run(WorkflowCommand* command);

    /** @brief Executes a FAILED command again. @return false if it is unknown or not FAILED. */
    bool retry(WorkflowCommand* command);

    /** @brief Undoes every DONE command, newest first, and marks it UNDONE. */
    void compensate();

    /** @brief Commands run so far, oldest first (owned by the processor). */
    const std::vector<WorkflowCommand*>& getHistory() const { return history; }

private:
    /** @brief One execute() call: counts the attempt, sets the status, raises the event. */
    bool attempt(WorkflowCommand* command);

    /** @brief Whether this command is already in the history. */
    bool owns(WorkflowCommand* command) const;

    WorkflowInstance* instance;                ///< Receives events (not owned).
    std::vector<WorkflowCommand*> history;     ///< Owned commands, oldest first.

    CommandProcessor(const CommandProcessor&);              ///< Not copyable: owns its commands.
    CommandProcessor& operator=(const CommandProcessor&);   ///< Not assignable.
};
