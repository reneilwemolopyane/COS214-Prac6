#ifndef MEMENTO_H
#define MEMENTO_H

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <cstddef>

#include "core/Common.h"

class WorkflowInstance;

/**
 * @brief Saved execution details for one workflow stage.
 *
 * Used as part of an instance snapshot so stage progress can be restored.
 */
struct StageMemento {
    std::string path;          ///< Path identifying the stage in the workflow.
    bool activated;            ///< Whether this stage has been activated.
    bool cancelledFlag;        ///< Whether this stage has been cancelled.
    bool finishedNotified;     ///< Whether completion has been reported.
    std::size_t cursor;        ///< Position of the active item in the stage.
    bool cursorActivated;      ///< Whether the cursor's item has been activated.
};

/**
 * @brief Saved execution details for one work item.
 *
 * Captures the item's state and associated result information when an
 * instance memento is created.
 */
struct WorkItemMemento {
    std::string id;                                  ///< Identifier of the work item.
    std::string stateName;                           ///< Name of the item's state.
    std::map<std::string, std::string> data;         ///< Data associated with the item.
    std::string outcome;                             ///< Outcome recorded for the item.
    std::string failureReason;                       ///< Reason recorded if the item failed.
};

/**
 * @brief Immutable snapshot of workflow instance data and state.
 *
 * A memento records workflow data, a state description, a version, and the
 * time at which the snapshot was created. WorkflowInstance is a friend and
 * can also populate the saved work-item and stage details.
 */
class InstanceMemento {
    friend class WorkflowInstance;

public:
    /**
     * @brief Create a snapshot of the supplied workflow state.
     * @param data Workflow data to capture.
     * @param stateSnapshot Textual description of the state to capture.
     * @param version Version associated with this snapshot.
     *
     * The timestamp is set to the current system time.
     */
    explicit InstanceMemento(const DataMap& data = DataMap(),
                              const std::string& stateSnapshot = std::string(),
                              int version = 0);

    /** @return The workflow data captured in this snapshot. */
    const DataMap& getData() const;

    /** @return The textual state description captured in this snapshot. */
    const std::string& getStateSnapshot() const;

    /** @return The version associated with this snapshot. */
    int getVersion() const;

    /** @return The time at which this snapshot was created. */
    std::chrono::system_clock::time_point getTimestamp() const;

    /**
     * @brief Format the creation time as local time.
     * @return A timestamp in `YYYY-MM-DD HH:MM:SS` format, or an empty string
     *         if the time cannot be formatted.
     */
    std::string getTimestampString() const;

private:
    DataMap data;
    std::string stateSnapshot;
    int version;
    std::chrono::system_clock::time_point timestamp;
    std::vector<WorkItemMemento> workItems;
    std::vector<StageMemento> stages;
};

#endif