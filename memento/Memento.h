#pragma once
#include <vector>
#include "tividy/core/Common.h"
/** @file Memento.h  Memento. NOTE: generic names (InstanceMemento). STUB */
class InstanceMemento {
    friend class WorkflowInstance;
    DataMap data; std::string stateSnapshot;
};
class ProcessDataStore {            // Caretaker, append-only
public:
    void append(InstanceMemento*) {}                    // TODO
    InstanceMemento* latest() const { return 0; }       // TODO
private: std::vector<InstanceMemento*> log;
};
