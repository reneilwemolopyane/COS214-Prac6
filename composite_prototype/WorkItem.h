#pragma once
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/Mediator.h"
#include "tividy/state/WorkItemState.h"

class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    explicit WorkItem(const std::string& name = "");
    ~WorkItem();

    
    void makeAvailable();
    void assign();
    void assign(Participant* p);
    void start();
    void complete();
    void suspend();
    void resume();


    void reject(const std::string& reason);
    void escalate(const std::string& reason);
    void cancel();

    
    bool isPending() const;
    bool isAvailable() const;
    bool isComplete() const;
    bool isFinished() const;
    bool isFailed() const;

    // Component Overrides
    void activate() override;
    WorkflowComponent* clone() const override;
    std::string getName() const override;
    void accept(WorkflowVisitor&) override;

    
    void setState(WorkItemState* s);
    const WorkItemState* getState() const;
    std::string getStateName() const;

    std::string getId() const { return id; }
    std::string getRole() const { return role; }
    Participant* getAssignee() const { return assignee; }
    
private:
    void releaseAssignee();

    std::string id;
    std::string name;
    std::string role;
    std::string failureReason;
    Participant* assignee;
    WorkItemState* state;
};