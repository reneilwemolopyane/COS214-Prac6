#include "tividy/memento/Memento.h"

#include <ctime>

InstanceMemento::InstanceMemento(const DataMap& data_,
                                 const std::string& stateSnapshot_,
                                 int version_)
    : data(data_),
      stateSnapshot(stateSnapshot_),
      version(version_),
      timestamp(std::chrono::system_clock::now()) {}

const DataMap& InstanceMemento::getData() const {
    return data;
}

const std::string& InstanceMemento::getStateSnapshot() const {
    return stateSnapshot;
}

int InstanceMemento::getVersion() const {
    return version;
}

std::chrono::system_clock::time_point InstanceMemento::getTimestamp() const {
    return timestamp;
}

std::string InstanceMemento::getTimestampString() const {
    const std::time_t timestampValue = std::chrono::system_clock::to_time_t(timestamp);
    const std::tm* localTime = std::localtime(&timestampValue);
    char buffer[80];
    if (!localTime || std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime) == 0) {
        return std::string();
    }
    return std::string(buffer);
}