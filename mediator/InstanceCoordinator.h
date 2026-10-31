#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>
#include "tividy/core/Common.h"

#include "tividy/mediator/WorkflowMediator.h"

class WorkItem;
class Participant;
class WorkAssigner;
class WorkflowSubject;
class Expression;
class ApprovalRouter;


/** @brief One prerequisite link: `item` cannot start until `prerequisite` is completed. */
struct Dependency {
    std::string item;
    std::string prerequisite;
};

/** @brief ConcreteMediator: one per WorkflowInstance. Holds dependency state. 
 * Holds the instance's dependency state, decides what an event means
 * (release next items, fail, escalate) and publishes events to observers.
 * It owns nothing: items, participants and collaborators belong to
 * WorkflowInstance or the caller
*/

class InstanceCoordinator : public WorkflowMediator{
    private:
       void closeItem(const std::string& id, const std::string& reason);
       void handleCompleted(const std::string& id);
       void handleRejection(const std::string& id);    
       void handleEscalation(const std::string& id);   
       void publish(const WorkflowEvent& e) const;
       void skipItem(WorkItem* item, const std::string& reason);
       bool onlySkippedPrerequisites(const std::string& itemId) const;
       void routeApproval(WorkItem& item);
       double amountFor(const std::string& itemId) const;

       std::vector<WorkItem*> items;
       std::vector<Dependency> dependencies;       
       std::set<std::string> closedItems;      ///< Items that were rejected, cancelled or escalated.
       std::vector<Dependency> orderings;
       std::set<std::string> completedItems;   ///< Items that completed successfully.
       std::set<std::string> skippedItems;     ///< Items whose branch was not taken (guard false or dead path).
       std::map<std::string, Expression*> guards;          ///< Owned. Interpreter rule per item.
       std::map<std::string, ApprovalRouter*> routers;     ///< Owned. Approval chain per item.
       std::map<std::string, std::string> amountKeys;      ///< Data key holding the approval amount.
       std::map<std::string, std::string> escalationRoles; ///< Who takes over when nobody responds.
       long now;                               ///< Simulated clock (seconds).
       std::map<std::string, long> deadlines;  ///< Per-item pickup deadline (seconds).
       std::map<std::string, long> dueAt;      ///< When an assigned item becomes overdue.
       void armDeadline(const std::string& itemId);
       bool canEscalate(const std::string& itemId) const { return routers.count(itemId) || escalationRoles.count(itemId); }
       const DataMap* data;                    ///< The instance's data (not owned); read by guards.
       std::vector<Participant*> pool;             
       WorkAssigner* assigner;
       WorkflowSubject* events; 
       bool started;  
                            
    public:
       InstanceCoordinator(WorkAssigner* assigner, WorkflowSubject* events);
       ~InstanceCoordinator();   ///< Deletes the guards and approval routers it owns.
       InstanceCoordinator& operator=(const InstanceCoordinator&) = delete;
 
       /** @brief Mediator entry point: publish first, then react */

       void notify(WorkflowColleague* sender, const WorkflowEvent& event) override;

       /** @brief Adds an item and makes this coordinator its mediator. @return false if null or duplicate id */
       bool registerItem(WorkItem* item);
       /** @brief Records that @p item cannot start until @p prerequisite has completed. */
       /** @brief Gate from a SEQUENTIAL stage: the item waits, but a skipped gate never makes it a dead path. */
       void addOrdering(const std::string& before, const std::string& after);
       void addDependency(const std::string& prerequisite, const std::string& item);
       void addParticipant(Participant* p);

       /** @brief Instance data the guards read (not owned; must outlive the coordinator). */
       void setData(const DataMap* d) { data = d; }
       /** @brief Attaches a branch rule (Interpreter text) to an item. @throws std::invalid_argument on bad syntax. */
       void setGuard(const std::string& itemId, const std::string& rule);
       /** @brief Routes the item's approval through a Chain of Responsibility built from @p levels. */
       void setApproval(const std::string& itemId, const std::string& amountKey, const std::string& levels);
       /** @brief Role that takes over the item when nobody responds. */
       void setEscalationRole(const std::string& itemId, const std::string& role) { escalationRoles[itemId] = role; }
       /**
        * @brief No response in time: moves the item to the next approval level (or the configured
        *        escalation role), or, when neither exists, escalates and closes it.
        * @return false if there is nowhere higher to go or nobody to hand it to.
        */
       bool escalate(const std::string& itemId, const std::string& reason);
       /** @brief Simulated time (seconds) used for deadlines. */
       void setClock(long seconds) { now = seconds; }
       /** @brief Seconds an assigned item may wait for pickup before it is escalated. */
       void setDeadline(const std::string& itemId, long seconds) { deadlines[itemId] = seconds; }
       /** @brief Escalates every assigned item whose deadline has passed. @return how many moved up. */
       int checkDeadlines();
       bool isSkipped(const std::string& itemId) const { return skippedItems.count(itemId) > 0; }
       /** @brief Name of the approval level currently responsible for the item, or "". */
       std::string approvalLevel(const std::string& itemId) const;

       /** @brief Every registered item, in registration order, whatever its state. */
       const std::vector<WorkItem*>& getItems() const { return items; }
       /** @brief The registered item with this id, or null. */
       WorkItem* findItem(const std::string& id) const;

       /** @brief Publishes CREATED and releases every item with no open prerequisites. */
       void start();
       bool dependenciesMet(const std::string& itemId) const;
       void releaseAvailableItems();
       std::vector<WorkItem*> getAvailableItems() const;

       /** @brief True when every item has completed or been closed (rejected, cancelled, escalated). */
       bool isFinished() const;
       /** @brief True when every item has completed successfully. */
       bool allCompleted() const;

       /** Rebuild completion/closure indexes after a memento restores item states. */
       void synchronizeAfterRestore();

    };

