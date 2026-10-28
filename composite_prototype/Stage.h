#pragma once
#include <vector>
#include <cstddef>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/core/Common.h"

class Stage : public WorkflowComponent {
public:
    explicit Stage(const std::string& name = "", ExecutionMode m = SEQUENTIAL);
    ~Stage() override;

    // Block shallow copies
    Stage(const Stage&) = delete;
    Stage& operator=(const Stage&) = delete;

    void add(WorkflowComponent* c);
    void remove(WorkflowComponent* c);
    std::size_t childCount() const;
    WorkflowComponent* childAt(std::size_t index) const;
    ExecutionMode getMode() const { return mode; }

    void start() override;
    void activate() override;
    void complete() override;
    void cancel() override;
    bool isComplete() const override;
    bool isFinished() const override;
    bool isFailed() const override;
    void childFinished(WorkflowComponent& child) override;
    WorkflowComponent* clone() const override;
    std::string getName() const override;
    void accept(WorkflowVisitor& v) override;

protected:
    std::string name;
    ExecutionMode mode;
    std::vector<WorkflowComponent*> children;
    
    bool activated;
    bool cancelledFlag;
    bool finishedNotified;
    bool busy;
    std::size_t cursor;
    bool cursorActivated;

    void advance();
    void finish();
    void cloneChildrenInto(Stage& target) const;
    
    struct BusyGuard {
        bool& busy;
        BusyGuard(bool& b) : busy(b) { busy = true; }
        ~BusyGuard() { busy = false; }
    };
};

class SubProcess : public Stage {
public:
    explicit SubProcess(const std::string& n = "", ExecutionMode m = SEQUENTIAL);
    WorkflowComponent* clone() const override;
};