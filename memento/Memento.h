#ifndef MEMENTO_H
#define MEMENTO_H

#include <chrono>
#include <string>
#include <map>
#include <vector>
#include <cstddef>

#include "core/Common.h"

class WorkflowInstance;

struct StageMemento {
    std::string path;
    bool activated;
    bool cancelledFlag;
    bool finishedNotified;
    std::size_t cursor;
    bool cursorActivated;
};

/** State of one work item at the instant the memento was created. */
struct WorkItemMemento {
    std::string id;
    std::string stateName;
    std::map<std::string, std::string> data;
    std::string outcome;
    std::string failureReason;
};

class InstanceMemento {
    friend class WorkflowInstance;

public:
    explicit InstanceMemento(const DataMap& data = DataMap(),
                              const std::string& stateSnapshot = std::string(),
                              int version = 0);

    const DataMap& getData() const;
    const std::string& getStateSnapshot() const;
    int getVersion() const;
    std::chrono::system_clock::time_point getTimestamp() const;
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