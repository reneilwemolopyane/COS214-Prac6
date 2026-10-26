#pragma once
#include <vector>
#include <cstddef>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/core/Common.h"

class Stage : public WorkflowComponent {
public:
    explicit Stage(const std::string& name = "", ExecutionMode m = SEQUENTIAL);
    ~Stage();
    void add(WorkflowComponent*);
    void start();
    void complete();
    void cancel();
    bool isComplete() const;
    WorkflowComponent* clone() const;
    std::string getName() const;
    void accept(WorkflowVisitor&);
protected:
    std::string name;
    ExecutionMode mode;
    std::vector<WorkflowComponent*> children;
    std::size_t cursor;
    bool cancelled;
};

class SubProcess : public Stage {
public:
    explicit SubProcess(const std::string& n = "");
    WorkflowComponent* clone() const;
};