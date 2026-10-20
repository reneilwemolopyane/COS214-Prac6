#pragma once
#include <vector>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/core/Common.h"
/** @brief Composite: groups children; SubProcess = nested Stage. STUB */
class Stage : public WorkflowComponent {
public:
    explicit Stage(const std::string& name = "", ExecutionMode m = SEQUENTIAL) : name(name), mode(m) {}
    void add(WorkflowComponent*) {}                 // TODO
    void start() {}                                 // TODO sequential/parallel
    void complete() {}                              // TODO
    void cancel() {}                                // TODO
    bool isComplete() const { return false; }       // TODO
    WorkflowComponent* clone() const { return 0; }  // TODO deep copy
    std::string getName() const { return name; }
    void accept(WorkflowVisitor&) {}                // TODO traverse children
protected:
    std::string name; ExecutionMode mode; std::vector<WorkflowComponent*> children;
};
class SubProcess : public Stage {
public:
    explicit SubProcess(const std::string& n = "") : Stage(n, SEQUENTIAL) {}
    WorkflowComponent* clone() const { return 0; }  // TODO
};
