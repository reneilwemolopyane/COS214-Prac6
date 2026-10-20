#pragma once
#include <vector>
#include "tividy/command/Command.h"
class WorkflowInstance;
/** @brief Command Invoker. One per instance. STUB */
class CommandProcessor {
public:
    explicit CommandProcessor(WorkflowInstance* i = 0) : instance(i) {}
    bool run(WorkflowCommand*) { return false; }     // TODO execute, record, setState
    bool retry(WorkflowCommand*) { return false; }   // TODO
    void undoAll() {}                                // TODO reverse order
private:
    WorkflowInstance* instance; std::vector<WorkflowCommand*> history;
};
