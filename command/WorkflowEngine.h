#pragma once
/**
 * @file WorkflowEngine.h
 * @brief Runs workflow instances. Facade subsystem class and Command pattern Client.
 */
#include <map>
#include <string>
#include <vector>
#include "core/WorkflowInstance.h"
#include "adapter/ConnectorResult.h"

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

    /** @brief Number of registered instances. */
    std::size_t instanceCount() const { return instances.size(); }

    /** @brief Assigns a work item through an AssignWorkItemCommand. @return true on success. */
    bool assignWorkItem(WorkflowInstance* instance, WorkItem* item, Participant* who);

    /** @brief Completes a work item through a CompleteWorkItemCommand. @return true on success. */
    bool completeWorkItem(WorkflowInstance* instance, WorkItem* item, const std::string& outcome);

    /**
     * @brief Calls an external system through an InvokeServiceCommand.
     * @param workItemId Work item the call belongs to, used in events.
     * @param undoOperation Operation that reverses the call (e.g. "reverse" for "pay"); empty if it cannot be undone.
     * @return true if the call ends up succeeding (including after retries).
     */
    bool invokeService(WorkflowInstance* instance, ExternalConnector* connector,
                       const ConnectorRequest& request, const std::string& workItemId = "",
                       const std::string& undoOperation = "");

private:
    /** @brief Runs a command on a RUNNING instance. Deletes the command if it cannot be run. */
    bool runCommand(WorkflowInstance* instance, WorkflowCommand* command);

    /** @brief Whether the instance is registered here and RUNNING. */
    bool isRunning(WorkflowInstance* instance) const;

    std::vector<WorkflowInstance*> instances;    ///< Owned instances, in registration order.
    std::map<std::string, std::string> status;   ///< Instance id -> lifecycle status.

    WorkflowEngine(const WorkflowEngine&);              ///< Not copyable: owns its instances.
    WorkflowEngine& operator=(const WorkflowEngine&);   ///< Not assignable.
};