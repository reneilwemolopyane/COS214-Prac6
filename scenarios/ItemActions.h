#ifndef ITEMACTIONS_H
#define ITEMACTIONS_H
/**
 * @file ItemActions.h
 * @brief The things a user can do to a work item, looked up by name in one table.
 *
 * The TUI and the web API both call run(); neither contains its own if/else chain over action names.
 * Each action is a small function that forwards to the engine (which wraps it in a Command).
 */
#include <string>
#include <vector>
#include "tividy/command/WorkflowEngine.h"

class ItemActions {
public:
    /** @return names of all actions, in menu order: start, complete, reject, escalate, service. */
    static std::vector<std::string> names();
    /** @return what the extra argument means for this action ("outcome", "reason", or "" if unused). */
    static std::string argumentLabel(const std::string& action);
    /**
     * @brief Runs one action on an item through the engine.
     * @param argument outcome for "complete", reason for "reject"/"escalate"; empty selects a default.
     * @return true on success; on refusal/failure see WorkflowEngine::getLastError().
     * @throws std::invalid_argument for an unknown action name.
     */
    static bool run(const std::string& action, WorkflowEngine& engine, WorkflowInstance* instance,
                    WorkItem* item, const std::string& argument);
};
#endif
