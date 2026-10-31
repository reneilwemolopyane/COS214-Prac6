#pragma once
#include <string>
#include <map>
#include <vector>
#include <utility>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/WorkflowMediator.h" 
#include "tividy/state/WorkItemState.h"
#include "tividy/state/States.h"

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
    using WorkflowComponent::complete;
    void complete(const std::string& o) override { outcome = o; complete(); }
    std::string getOutcome() const { return outcome; }
    void reopen() { setState(InProgressState::instance()); }
    void reject(const std::string& reason);
    void cancel() override;
    void escalate(const std::string& reason);
    void suspend();
    void resume();

    
    void markEscalated(); 

    /** @brief Branch not taken (guard false / dead path): Created -> Skipped. Notifies the mediator. */
    void skip(const std::string& reason);
    /** @brief Hands an escalated item to another participant: Assigned -> Escalated -> Assigned. */
    void escalateTo(Participant* next);
    bool isSkipped() const;
    WorkItemState::Disposition getDisposition() const;   ///< open / done / not needed / closed
    bool wasRejected() const;
    bool isInProgress() const;
    bool isWaitingForPickup() const;   ///< Assigned but not started.

    
    void activate() override;
    bool isComplete() const override;
    bool isFinished() const override;
    WorkItem* underlyingItem() override { return this; }
    void collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& out) override;
    bool isFailed() const override;
    WorkflowComponent* clone() const override;
    std::string getName() const override;
    void accept(WorkflowVisitor& v) override;

    bool isPending() const;
    bool isAvailable() const;
    std::string getStateName() const;
    const std::map<std::string, std::string>& getAllData() const { return data; }
    const std::string& getFailureReason() const { return failureReason; }
    void restoreSnapshot(const std::string& stateName,
                         const std::map<std::string, std::string>& snapshotData,
                         const std::string& snapshotOutcome,
                         const std::string& snapshotFailureReason);
    const WorkItemState* getState() const;

    std::string getId() const { return id; }
    std::string getRole() const { return role; }
    Participant* getAssignee() const { return assignee; }
    std::string getAssignmentRule() const { return assignmentRule; } // NEW
    /** @brief Role a participant must hold to be assigned this item (used by the assignment strategies). */
    void setRole(const std::string& r) { role = r; }
    /** @brief Name of the assignment rule (strategy) the WorkAssigner should use for this item; empty = default. */
    void setAssignmentRule(const std::string& rule) { assignmentRule = rule; }
    
    std::string getData(const std::string& key) const;               // NEW
    void setData(const std::string& key, const std::string& value);  // NEW
    
    void setState(WorkItemState* s);
    void releaseAssignee();
    /** @brief Puts @p p back as assignee WITHOUT a state change (used when a memento restores the item). */
    void adoptAssignee(Participant* p);

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