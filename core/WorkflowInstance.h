#pragma once
#include <string>
#include <map>
#include <utility>
#include <vector>
#include "tividy/core/ItemConfig.h"
#include "tividy/observer/ObserverPattern.h"
#include "tividy/visitor/Visitor.h"
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/command/CommandProcessor.h"
#include "tividy/memento/Memento.h"

class WorkflowInstance;
class AuditLogger;
class DependencyTracker;
class NotificationService;
class IncidentMonitor;
class Participant;
class WorkAssigner;
class WorkflowInstance : public WorkflowSubject, public WorkflowElement {
public:
    WorkflowInstance(const std::string& id,
                     WorkflowComponent* rootCopy,
                     const DataMap& d,
                     int version = 1);

    ~WorkflowInstance();

    /**
     * @brief Registers the items, hands the dependencies and participants to the
     *        coordinator and releases every item whose prerequisites are met.
     *
     * Release is driven by the dependencies (InstanceCoordinator), not by the
     * Stage activation order: stages group the items, dependencies order them.
     */
    void start();

    /** @brief Records that @p item cannot start until @p prerequisite has completed. Call before start(). */
    void addDependency(const std::string& prerequisite, const std::string& item);
    /** @brief Ordering implied by a SEQUENTIAL stage: only gates the item, never decides a skip. */
    void addOrdering(const std::string& before, const std::string& after) { orderings.push_back(std::make_pair(before, after)); }

    /** @brief Adds a participant to the pool the assigner chooses from (not owned). Call before start(). */
    void addParticipant(Participant* participant);

    /** @brief Per-item settings from the definition (guards, approval chains, decorators, services). Call before start(). */
    void setItemConfigs(const std::map<std::string, ItemConfig>& configs) { itemConfigs = configs; }
    /** @return the settings for this item (empty settings if the definition gave none). */
    ItemConfig getItemConfig(const std::string& itemId) const;

    /** @brief Instance data (the payload). Guards and approval chains read it. */
    std::string getData(const std::string& key) const;
    void setData(const std::string& key, const std::string& value) { data[key] = value; }
    const DataMap& getAllData() const { return data; }
    /** @brief Who is acting right now. Security decorators and the engine check it. Not owned. */
    void setActor(Participant* who) { actor = who; }
    /** While true, the engine itself is acting (cancel / compensation): role checks of the security decorator are satisfied. */
    void setSystemActing(bool on) { systemActing = on; }
    /** @brief Simulated clock (seconds) given to the coordinator; set before start(). */

    void setClock(long seconds) { clockSeconds = seconds; if (coordinator) coordinator->setClock(seconds); }
    Participant* getActor() const { return actor; }

    /** @brief The outermost component for an item: the decorator stack if it has one, else the item itself. */
    WorkflowComponent* frontOf(const std::string& itemId) const;

    /** @brief Lines recorded by audit and notification decorators. */
    const std::vector<std::string>& getDecoratorLog() const { return decoratorLog; }

    /** @brief IN_PROGRESS, COMPLETED, REJECTED, CANCELLED or CLOSED (stopped with unfinished work). */
    std::string getOutcome(bool cancelled) const;

    /** @brief Cancels every work item that has not finished yet. Used when the whole instance is cancelled. */
    void cancelAll();

    /** @brief True once every work item has completed or been closed (rejected, cancelled, escalated). */
    bool isFinished() const;

    /** @brief True once every work item has completed successfully. */
    bool isCompleted() const;
    void accept(WorkflowVisitor&);
    InstanceMemento* createMemento() const;
    void restoreFromMemento(const InstanceMemento&);

    std::string getId() const { return id; }
    int getVersion() const { return version; }

    CommandProcessor* getProcessor() { return processor; }
    InstanceCoordinator* getCoordinator() { return coordinator; }

    /**
     * @brief Builds a WorkflowEvent for this instance and publishes it to the observers.
     * Called by the CommandProcessor and the WorkflowEngine.
     * @param type Event type.
     * @param workItemId Work item involved, if any.
     * @param cause Command that caused the event, or null (not owned).
     * @param details Extra information, e.g. an error message.
     */
    /**
     * @brief Completes the event with this instance's id and source, then pushes it to the observers.
     * Events raised by work items and the coordinator carry no instance info; this adds it.
     */
    void notifyObservers(const WorkflowEvent& event) override;

    /** @brief Built-in observers, created with the instance and attached to the right events. */
    AuditLogger* getAuditLogger() { return audit; }
    DependencyTracker* getDependencyTracker() { return tracker; }
    NotificationService* getNotificationService() { return notifier; }
    IncidentMonitor* getIncidentMonitor() { return monitor; }

    void raiseEvent(EventType type, const std::string& workItemId = "",
                    WorkflowCommand* cause = 0, const std::string& details = "");

    WorkflowComponent* getRoot() const { return root; }

private:
    std::string id;
    WorkflowComponent* root;
    DataMap data;
    int version;
    InstanceCoordinator* coordinator;
    CommandProcessor* processor;
    AuditLogger* audit;              ///< Owned built-in observers.
    DependencyTracker* tracker;
    NotificationService* notifier;
    IncidentMonitor* monitor;
    WorkAssigner* assigner;                                         ///< Owned; created in start().
    std::vector<Participant*> participants;                         ///< Not owned.
    std::vector<std::pair<std::string, std::string> > dependencies; ///< (prerequisite, item).
    std::vector<std::pair<std::string, std::string> > orderings;    ///< from SEQUENTIAL stages.
    std::map<std::string, ItemConfig> itemConfigs;
    std::map<std::string, WorkflowComponent*> fronts;               ///< item id -> outermost component (not owned).
    std::vector<std::string> decoratorLog;
       long clockSeconds;
    Participant* actor;                                             ///< Not owned.
    bool systemActing;                                              ///< True during engine-initiated cancel/compensation.

    void applyDecorations();

};
