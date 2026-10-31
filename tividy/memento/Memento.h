#ifndef MEMENTO_H
#define MEMENTO_H

#include <chrono>
#include <string>
#include <map> // NEW
#include "tividy/core/Common.h"

class WorkflowInstance;

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

    // NEW: Expose item states for auditing
    const std::map<std::string, std::string>& getItemStates() const { return itemStates; }

private:
    DataMap data;
    std::string stateSnapshot;
    int version;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> itemStates; // NEW: ID -> StateName
};

#endif