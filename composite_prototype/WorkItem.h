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

/**
 * @brief Leaf workflow component representing an individually assigned unit of work.
 *
 * A work item delegates transition validation to its current state and reports
 * lifecycle events to its mediator when one is attached.
 */
class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    
    /**
     * @brief Creates an item in the Created state.
     * @param id Stable identifier.
     * @param name Human-readable name.
     */
    explicit WorkItem(const std::string& id, const std::string& name);
    /** @brief Releases any assigned participant. */
    ~WorkItem() override;

    /** @brief Transitions the item to Available. */
    void makeAvailable();
    /** @brief Assigns the item through its current state. */
    void assign();
    /** @brief Assigns the item to a specific participant. */
    void assign(Participant* p);
    /** @brief Starts work on the item. */
    void start() override;
    /** @brief Completes the item and notifies its parent and mediator. */
    void complete() override;
    using WorkflowComponent::complete;
    /** @brief Records an outcome and completes the item. */
    void complete(const std::string& o) override { outcome = o; complete(); }
    /** @return The recorded completion outcome. */
    std::string getOutcome() const { return outcome; }
    /** @brief Reopens the item by moving it to InProgress. */
    void reopen() { setState(InProgressState::instance()); }
    /** @brief Rejects the item with a failure reason. */
    void reject(const std::string& reason);
    /** @brief Cancels the item. */
    void cancel() override;
    /** @brief Escalates the item with a reason. */
    void escalate(const std::string& reason);
    /** @brief Suspends active work. */
    void suspend();
    /** @brief Resumes suspended work. */
    void resume();

    
    /** @brief Releases the assignee and moves the item to Escalated. */
    void markEscalated(); 

    /** @brief Branch not taken (guard false / dead path): Created -> Skipped. Notifies the mediator. */
    void skip(const std::string& reason);
    /** @brief Hands an escalated item to another participant: Assigned -> Escalated -> Assigned. */
    void escalateTo(Participant* next);
    /** @return Whether the item is in the Skipped state. */
    bool isSkipped() const;
    /** @return The high-level disposition of the item's current state. */
    WorkItemState::Disposition getDisposition() const;
    /** @return Whether the item was rejected. */
    bool wasRejected() const;
    /** @return Whether the item is currently InProgress. */
    bool isInProgress() const;
    /** @return Whether the item is assigned but has not started. */
    bool isWaitingForPickup() const;

    
    /** @brief Makes a pending item available when activated by its parent. */
    void activate() override;
    /** @return Whether the item completed successfully or was skipped. */
    bool isComplete() const override;
    /** @return Whether the item is in a terminal state. */
    bool isFinished() const override;
    /** @return This leaf work item. */
    WorkItem* underlyingItem() override { return this; }
    /** @brief Adds this item and its component to the collected item list. */
    void collectItems(std::vector<std::pair<WorkItem*, WorkflowComponent*> >& out) override;
    /** @return Whether the item was rejected. */
    bool isFailed() const override;
    /** @brief Creates a copy of the item's definition in the Created state. */
    WorkflowComponent* clone() const override;
    /** @return The item's name. */
    std::string getName() const override;
    /** @brief Dispatches this item to a visitor. */
    void accept(WorkflowVisitor& v) override;

    /** @return Whether the item is in the Created state. */
    bool isPending() const;
    /** @return Whether the item is in the Available state. */
    bool isAvailable() const;
    /** @return The current state name. */
    std::string getStateName() const;
    /** @return All item data key/value pairs. */
    const std::map<std::string, std::string>& getAllData() const { return data; }
    /** @return The reason recorded for rejection, cancellation, or escalation. */
    const std::string& getFailureReason() const { return failureReason; }
    /**
     * @brief Restores persisted item state without emitting workflow events.
     * @throws std::invalid_argument if stateName is not a known work-item state.
     */
    void restoreSnapshot(const std::string& stateName,
                         const std::map<std::string, std::string>& snapshotData,
                         const std::string& snapshotOutcome,
                         const std::string& snapshotFailureReason);
    /** @return The current state object. */
    const WorkItemState* getState() const;

    /** @return The stable item identifier. */
    std::string getId() const { return id; }
    /** @return The role required to be assigned this item. */
    std::string getRole() const { return role; }
    /** @return The assigned participant, or nullptr if unassigned. */
    Participant* getAssignee() const { return assignee; }
    /** @return The name of the assignment rule, or an empty string for the default. */
    std::string getAssignmentRule() const { return assignmentRule; } // NEW
    /** @brief Role a participant must hold to be assigned this item (used by the assignment strategies). */
    void setRole(const std::string& r) { role = r; }
    /** @brief Name of the assignment rule (strategy) the WorkAssigner should use for this item; empty = default. */
    void setAssignmentRule(const std::string& rule) { assignmentRule = rule; }
    
    /** @return The value for key, or an empty string if it is not set. */
    std::string getData(const std::string& key) const;
    /** @brief Sets or replaces a data value. */
    void setData(const std::string& key, const std::string& value);
    
    /** @brief Sets the current state when the supplied state pointer is non-null. */
    void setState(WorkItemState* s);
    /** @brief Releases the current participant assignment, if any. */
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