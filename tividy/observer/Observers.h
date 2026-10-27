#pragma once
/**
 * @file Observers.h
 * @brief Observer pattern ConcreteObservers.
 *
 * Each observer receives the pushed WorkflowEvent and reacts only to the
 * event types it cares about. Dependency handling is not done here; it
 * belongs to the Mediator (InstanceCoordinator).
 */
#include <string>
#include <vector>
#include "tividy/observer/ObserverPattern.h"

class CommandProcessor;
class ExternalConnector;

/**
 * @brief ConcreteObserver that records every event in the instance's audit trail.
 */
class AuditLogger : public WorkflowObserver {
public:
    /** @brief Attaches to the given subject. */
    explicit AuditLogger(WorkflowSubject* s) : WorkflowObserver(s) {}

    /** @brief Stores the event. Reacts to every event type. */
    void update(const WorkflowEvent& e);

    /** @brief All recorded events, oldest first. */
    const std::vector<WorkflowEvent>& getTrail() const { return trail; }

    /** @brief The trail as readable lines, e.g. "FAILED Pay [InvokeService] - service unavailable". */
    std::vector<std::string> lines() const;

private:
    std::vector<WorkflowEvent> trail;   ///< Append-only list of events.
};

/**
 * @brief ConcreteObserver that tells people when they must act.
 *
 * Reacts to ASSIGNED, ESCALATED and REJECTED. Messages are sent through an
 * ExternalConnector (Adapter pattern), e.g. an SMS or email adapter, as a
 * "send" request with params "to", "text" and "urgent".
 *
 * The recipient is taken from the event payload: "to" if present, otherwise
 * "assignee", otherwise the default recipient (e.g. a supervisor for escalations).
 * With no recipient the message is recorded but not sent.
 *
 * With a CommandProcessor, each message is sent as a SendNotificationCommand, so a
 * failed send is retried by the IncidentMonitor and appears in the audit trail.
 * Without one, the message is sent directly and failures are kept in getFailed().
 */
class NotificationService : public WorkflowObserver {
public:
    /**
     * @brief Attaches to the subject.
     * @param s Subject to observe.
     * @param channel Connector used to send messages. If null, messages are only recorded.
     * @param defaultRecipient Who gets a message when the event names nobody, e.g. "supervisor".
     * @param processor Invoker used to send messages as commands (not owned). Null sends directly.
     */
    NotificationService(WorkflowSubject* s, ExternalConnector* channel = 0,
                        const std::string& defaultRecipient = "", CommandProcessor* processor = 0)
        : WorkflowObserver(s), channel(channel), defaultRecipient(defaultRecipient), processor(processor) {}

    /** @brief Builds and sends a message for relevant event types. */
    void update(const WorkflowEvent& e);

    /** @brief Every message sent so far. */
    const std::vector<std::string>& getSent() const { return sent; }

    /** @brief Messages that failed when sent directly (no processor). */
    const std::vector<std::string>& getFailed() const { return failed; }

private:
    ExternalConnector* channel;     ///< Outgoing message channel (not owned).
    std::string defaultRecipient;   ///< Used when the event has no "to" or "assignee".
    CommandProcessor* processor;    ///< Sends messages as commands when set (not owned).
    std::vector<std::string> sent;  ///< Messages produced, for checking and reporting.
    std::vector<std::string> failed;  ///< Direct sends that failed.
};

/**
 * @brief ConcreteObserver that handles failures.
 *
 * On FAILED: retries the causing command through the CommandProcessor while
 * the failure is temporary and attempts remain; otherwise raises an incident
 * for a person to fix. On TIMED_OUT: raises an incident.
 */
class IncidentMonitor : public WorkflowObserver {
public:
    /**
     * @brief Attaches to the subject.
     * @param s Subject to observe.
     * @param p Processor used to retry failed commands (not owned).
     * @param maxAttempts Total attempts allowed before an incident is raised.
     */
    IncidentMonitor(WorkflowSubject* s, CommandProcessor* p, int maxAttempts = 3)
        : WorkflowObserver(s), processor(p), maxAttempts(maxAttempts) {}

    /** @brief Retries or raises an incident, depending on the event. */
    void update(const WorkflowEvent& e);

    /** @brief Incidents raised so far. */
    const std::vector<std::string>& getIncidents() const { return incidents; }

private:
    /** @brief Records an incident with the reason. */
    void raiseIncident(const WorkflowEvent& e, const std::string& reason);

    CommandProcessor* processor;           ///< Invoker used for retries (not owned).
    int maxAttempts;                       ///< Retry limit (the retry policy).
    std::vector<std::string> incidents;    ///< Open incidents.
};