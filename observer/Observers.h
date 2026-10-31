#pragma once
/**
 * @file Observers.h
 * @brief ConcreteObservers of the Observer pattern.
 *
 * Each observer is independent: it reacts only to the event types it was
 * attached for, and the subject does not know what any of them does.
 */
#include <string>
#include <vector>
#include "tividy/observer/ObserverPattern.h"

class CommandProcessor;
class ExternalConnector;

/**
 * @brief Records every event in the instance's audit trail.
 *
 * Attach it to all event types. Stored events keep their text fields but not
 * the source/cause pointers, which could dangle after the instance is gone.
 */
class AuditLogger : public WorkflowObserver {
public:
    void update(const WorkflowEvent& event) override;

    /** @brief Every event seen so far, oldest first. */
    const std::vector<WorkflowEvent>& getTrail() const { return trail; }

    /** @brief The trail as readable lines, e.g. "[2026-11-02 10:00:01] ASSIGNED inspect (assignee: Thandi)". */
    std::vector<std::string> getLines() const;

    /** @brief Formats one event as a single line. */
    static std::string format(const WorkflowEvent& event);

    /** @brief Number of events recorded. */
    std::size_t size() const { return trail.size(); }

private:
    std::vector<WorkflowEvent> trail;
};

/**
 * @brief Tracks which work items have completed and which have been closed
 *        (rejected, cancelled, escalated or failed without a command).
 *
 * The InstanceCoordinator (Mediator) does the actual releasing of dependent
 * items; this observer keeps the progress view that the UI and the visitors use.
 */
class DependencyTracker : public WorkflowObserver {
public:
    void update(const WorkflowEvent& event) override;

    const std::vector<std::string>& getCompleted() const { return completed; }
    const std::vector<std::string>& getClosed() const { return closed; }
    bool isCompleted(const std::string& workItemId) const;
    bool isClosed(const std::string& workItemId) const;
    std::size_t completedCount() const { return completed.size(); }
    std::size_t closedCount() const { return closed.size(); }

private:
    std::vector<std::string> completed;
    std::vector<std::string> closed;
};

/**
 * @brief Tells the participant who must act next, through an ExternalConnector.
 *
 * On ASSIGNED it sends "your work item is ready"; on ESCALATED it sends an urgent
 * message to the supervisor. Every message is also kept in getMessages(), so it
 * works (and can be tested) without a connector.
 */
class NotificationService : public WorkflowObserver {
public:
    /** @param connector Messaging connector (not owned), or null to only record messages. */
    explicit NotificationService(ExternalConnector* connector = 0) : connector(connector), failures(0) {}

    void update(const WorkflowEvent& event) override;

    void setConnector(ExternalConnector* c) { connector = c; }

    /** @brief Messages produced so far, as "to: text". */
    const std::vector<std::string>& getMessages() const { return messages; }

    /** @brief How many messages the connector refused or failed to deliver. */
    int getFailureCount() const { return failures; }

private:
    void send(const std::string& to, const std::string& text, bool urgent);

    ExternalConnector* connector;
    std::vector<std::string> messages;
    int failures;
};

/**
 * @brief On FAILED, asks the CommandProcessor to retry the failed command;
 *        when retries run out (or the failure is permanent) it raises an incident.
 */
class IncidentMonitor : public WorkflowObserver {
public:
    /**
     * @param processor Processor that owns the commands (not owned).
     * @param maxAttempts Total attempts allowed per command, including the first.
     */
    explicit IncidentMonitor(CommandProcessor* processor, int maxAttempts = 3)
        : processor(processor), maxAttempts(maxAttempts) {}

    void update(const WorkflowEvent& event) override;

    /** @brief Incidents raised so far, as readable lines. */
    const std::vector<std::string>& getIncidents() const { return incidents; }

private:
    CommandProcessor* processor;   ///< Not owned.
    int maxAttempts;
    std::vector<std::string> incidents;
};
