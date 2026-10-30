#pragma once
#include <string>
#include <map>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/WorkflowMediator.h" 
#include "tividy/state/WorkItemState.h"

class Participant; // FIXED: Forward declaration

class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    
    explicit WorkItem(const std::string& id, const std::string& name);
    ~WorkItem() override;

    void makeAvailable();
    void assign();
    void assign(Participant* p);
    void start() override;
    void complete() override;
    void complete(const std::string& outcome);
    void reopen();
    void reject(const std::string& reason);
    void cancel() override;
    void escalate(const std::string& reason);
    void suspend();
    void resume();
    void setRole(const std::string& r) { role = r; }
    void setAssignmentRule(const std::string& r) { assignmentRule = r; }
    
    void markEscalated(); 

    
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
    void setRole(const std::string& r);
    void setAssignmentRule(const std::string& r);
    Participant* getAssignee() const { return assignee; }
    std::string getOutcome() const { return outcome; }
    std::string getAssignmentRule() const { return assignmentRule; } // NEW
    
    std::string getData(const std::string& key) const;               // NEW
    void setData(const std::string& key, const std::string& value);  // NEW
    
    void setState(WorkItemState* s);
    void releaseAssignee();

private:
    std::string id;     
    std::string name;   
    std::string role;
    std::string failureReason;
    std::string outcome;
    Participant* assignee;
    WorkItemState* state;

    // NEW: Members for other patterns
    std::string assignmentRule;
    std::map<std::string, std::string> data; 
};