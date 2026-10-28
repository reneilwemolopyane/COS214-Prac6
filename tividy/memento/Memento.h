#ifndef MEMENTO_H
#define MEMENTO_H

#include <chrono>
#include <string>

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

private:
    DataMap data;
    std::string stateSnapshot;
    int version;
    std::chrono::system_clock::time_point timestamp;
};

#endif