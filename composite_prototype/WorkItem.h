#pragma once
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/Mediator.h"
#include "tividy/state/WorkItemState.h"

class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    // Fixed: Separate id and name
    explicit WorkItem(const std::string& id, const std::string& name);
    ~WorkItem() override;

    void makeAvailable();
    void assign();
    void assign(Participant* p);
    void start() override;
    void complete() override;
    void reject(const std::string& reason);
    void cancel() override;
    void escalate(const std::string& reason);
    void suspend();
    void resume();

    void activate() override;
    bool isComplete() const override;
    bool isFinished() const override;
    bool isFailed() const override;
    WorkflowComponent* clone() const override;
    std::string getName() const override;
    void accept(WorkflowVisitor& v) override;

    bool isPending() const;
    bool isAvailable() const;
    std::string getStateName() const;
    const WorkItemState* getState() const;

    std::string getId() const { return id; }
    std::string getRole() const { return role; }
    Participant* getAssignee() const { return assignee; }
    
    void setState(WorkItemState* s);
    void releaseAssignee();

private:
    std::string id;     
    std::string name;   
    std::string role;
    std::string failureReason;
    Participant* assignee;
    WorkItemState* state;
};