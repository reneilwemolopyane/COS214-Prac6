#pragma once
/**
 * @file WorkflowEngine.h
 * @brief Runs workflow instances. Facade subsystem class and Command pattern Client.
 */
#include <map>
#include <string>
#include <vector>
#include "tividy/core/WorkflowInstance.h"
#include "tividy/adapter/ConnectorResult.h"
#include "tividy/adapter/ConnectorRegistry.h"

class WorkItem;
class Participant;
class ExternalConnector;

/**
 * @brief Registers, starts, tracks and cancels workflow instances.
 *
 * Subsystem class behind WorkflowFacade: the facade finds a definition,
 * creates an instance and hands it to the engine, which then owns it.
 *
 * Also the Command pattern Client: every action on an instance is wrapped
 * in a concrete command and run by that instance's CommandProcessor, so it
 * is recorded, can be retried and can be compensated.
 */
class WorkflowEngine {
public:
    WorkflowEngine() {}

    /** @brief Deletes every registered instance. */
    ~WorkflowEngine();

    /**
     * @brief Takes ownership of an instance and marks it REGISTERED.
     * @return false if the instance is null or its id is already registered (the caller keeps ownership).
     */
    /**
     * @brief Adds a participant to the pool every started instance assigns work from.
     * The engine takes ownership. Participants added after an instance has started
     * are not given to that instance.
     * @return false if @p participant is null.
     */
    bool addParticipant(Participant* participant);

    /** @brief The participants the engine owns, in the order they were added. */
    const std::vector<Participant*>& getParticipants() const { return participants; }

    bool registerInstance(WorkflowInstance* instance);

    /**
     * @brief Starts a registered instance and raises a STARTED event.
     * @return false if the instance is unknown or not in REGISTERED state.
     */
    bool start(WorkflowInstance* instance);

    /** @brief Starts a registered instance by id. */
    bool start(const std::string& instanceId);

    /**
     * @brief Cancels a running instance: compensates its completed commands and raises CANCELLED.
     * @return false if the instance is unknown or not RUNNING.
     */
    bool cancel(const std::string& instanceId);

    /** @brief Instance with this id, or null. */
    WorkflowInstance* findInstance(const std::string& instanceId) const;

    /** @brief REGISTERED, RUNNING, CANCELLED, or NOT_FOUND. */
    std::string getStatus(const std::string& instanceId) const;

    /** @brief Every registered instance, in registration order (non-owning view). */
    const std::vector<WorkflowInstance*>& getInstances() const { return instances; }

    /** @brief Number of registered instances. */
    std::size_t instanceCount() const { return instances.size(); }

    /** @brief External systems (Adapter pattern) available to every instance, found by a configured name. */
    ConnectorRegistry& connectors() { return connectorRegistry; }

    /** @brief Why the last action was refused or failed ("" if it succeeded). */
    const std::string& getLastError() const { return lastError; }

    /** @brief COMPLETED, REJECTED, CANCELLED, CLOSED or IN_PROGRESS. */
    std::string getOutcome(const std::string& instanceId) const;

    /**
     * @brief Simulated time passes. Every assigned item whose deadline has expired is escalated
     *        (next approval level or configured role). @return how many items were escalated.
     */
    int advanceClock(long seconds);
    long getClock() const { return clockSeconds; }

    /** @brief Starts work on an assigned item through a StartWorkItemCommand. */
    bool startWorkItem(WorkflowInstance* instance, WorkItem* item);

    /**
     * @brief Calls the external system bound to this item by its definition (Adapter), e.g. a registry
     *        check or a payment, with retries. The answer is stored in the instance data as "<item>.<key>".
     * @return true if the item has no binding or the call succeeded.
     */
    bool callItemService(WorkflowInstance* instance, WorkItem* item);

    /** @brief Rejects an in-progress item with a reason (exception path). Refused if the item is not InProgress. */
    bool rejectWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& reason);

    /** @brief No response in time: moves the item to the next approval level / escalation role. */
    bool escalateWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& reason);

    /** @brief Assigns a work item through an AssignWorkItemCommand. @return true on success. */
    bool assignWorkItem(WorkflowInstance* instance, WorkItem* item, Participant* who);

    /** @brief Completes a work item through a CompleteWorkItemCommand. @return true on success. */
    bool completeWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& outcome);

    /**
     * @brief Calls an external system through an InvokeServiceCommand.
     * @param workItemId Work item the call belongs to, used in events.
     * @return true if the call ends up succeeding (including after retries).
     */
    bool invokeService(WorkflowInstance* instance, ExternalConnector* connector,
                       const ConnectorRequest& request, const std::string& workItemId = "",
                       const std::string& undoOperation = "reverse");

private:
    /** @brief Runs a command on a RUNNING instance. Deletes the command if it cannot be run. */
    bool runCommand(WorkflowInstance* instance, WorkflowCommand* command);

    /** @brief Whether the instance is registered here and RUNNING. */
    bool isRunning(WorkflowInstance* instance) const;

    /** @brief Checks that the acting participant is the item's assignee; sets lastError and raises REFUSED if not. */
    bool actorMayAct(WorkflowInstance* instance, WorkItem* item, const char* what);
    /** @brief Builds the connector request from the item's service binding and the instance data. */
    ConnectorRequest buildServiceRequest(WorkflowInstance* instance, const ItemConfig& cfg) const;
    void refuse(WorkflowInstance* instance, const std::string& itemId, const std::string& why);

    long clockSeconds = 0;                       ///< Simulated time shared by all instances.
    ConnectorRegistry connectorRegistry;         ///< Connectors are not owned (the scenario owns its adapters).
    std::string lastError;
    std::vector<Participant*> participants;      ///< Owned; deleted after the instances.
    std::vector<WorkflowInstance*> instances;    ///< Owned instances, in registration order.
    std::map<std::string, std::string> status;   ///< Instance id -> lifecycle status.

    WorkflowEngine(const WorkflowEngine&);              ///< Not copyable: owns its instances.
    WorkflowEngine& operator=(const WorkflowEngine&);   ///< Not assignable.
};
