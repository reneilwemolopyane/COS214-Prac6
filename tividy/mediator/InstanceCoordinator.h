#pragma once
#include <set>
#include <string>
#include <vector>

#include "tividy/mediator/WorkflowMediator.h"

class WorkItem;
class Participant;
class WorkAssigner;
class WorkflowSubject;


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
       WorkItem* findItem(const std::string& id) const;
       void closeItem(const std::string& id, const std::string& reason);
       void handleCompleted(const std::string& id);
       void handleRejection(const std::string& id);    
       void handleEscalation(const std::string& id);   
       void publish(const WorkflowEvent& e) const;

       std::vector<WorkItem*> items;
       std::vector<Dependency> dependencies;       
       std::vector<std::string> closedItems;    
       std::vector<Participant*> pool;             
       WorkAssigner* assigner;
       WorkflowSubject* events; 
       bool started;  
                            
    public:
       InstanceCoordinator(WorkAssigner* assigner, WorkflowSubject* events);
       ~InstanceCoordinator();
       InstanceCoordinator& operator=(const InstanceCoordinator&) = delete;
 
       /** @brief Mediator entry point: publish first, then react */

       void notify(Colleague* sender, WorkflowEvent event, const std::string& itemId) override;

       /** @brief Adds an item and makes this coordinator its mediator. @return false if null or duplicate id */
       void registerItem(WorkItem* item);          
       void addDependency(const std::string& item, const std::string& prerequisite); 
       void addParticipant(Participant* p);

       /** @brief Publishes CREATED and releases every item with no open prerequisites. */
       void start();
       bool dependenciesMet(const std::string& itemId) const;
       void releaseAvailableItems();
       std::vector<WorkItem*> getAvailableItems() const;  
    
    };

