/**
 * @file test_observer.cpp
 * @brief Unit tests for the Observer pattern (WorkflowSubject, WorkflowObserver and the concrete observers).
 */
#include "tests/doctest.h"
#include "tests/TestConnectors.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/observer/Observers.h"

TEST_CASE("Observers attach and detach themselves") {
    WorkflowInstance instance("I-1", 0, DataMap());

    SUBCASE("constructor attaches") {
        AuditLogger logger(&instance);
        CHECK(instance.observerCount() == 1);
    }

    SUBCASE("destructor detaches") {
        AuditLogger* logger = new AuditLogger(&instance);
        delete logger;
        CHECK(instance.observerCount() == 0);
    }

    SUBCASE("attaching the same observer twice is ignored") {
        AuditLogger logger(&instance);
        instance.attach(&logger);
        CHECK(instance.observerCount() == 1);
    }
}

TEST_CASE("Subject pushes the full event to observers (push model)") {
    WorkflowInstance instance("I-2", 0, DataMap());
    AuditLogger logger(&instance);

    instance.raiseEvent(EventType::ASSIGNED, "Review", 0, "assigned to Anna");

    REQUIRE(logger.getTrail().size() == 1);
    const WorkflowEvent& e = logger.getTrail()[0];
    CHECK(e.getType() == EventType::ASSIGNED);
    CHECK(e.getWorkItemId() == "Review");
    CHECK(e.get("message") == "assigned to Anna");
    CHECK(e.get("instance") == "I-2");
    CHECK(e.getSource() == &instance);
}

TEST_CASE("Observer that outlives its subject does not keep a dangling pointer") {
    WorkflowInstance* instance = new WorkflowInstance("I-3", 0, DataMap());
    AuditLogger logger(instance);
    delete instance;
    CHECK(logger.getSubject() == 0);
}

TEST_CASE("AuditLogger records every event in order") {
    WorkflowInstance instance("I-4", 0, DataMap());
    AuditLogger logger(&instance);

    instance.raiseEvent(EventType::STARTED);
    instance.raiseEvent(EventType::ASSIGNED, "Review");
    instance.raiseEvent(EventType::COMPLETED, "Review");

    REQUIRE(logger.getTrail().size() == 3);
    CHECK(logger.getTrail()[0].getType() == EventType::STARTED);
    CHECK(logger.getTrail()[2].getType() == EventType::COMPLETED);
    CHECK(logger.lines()[1] == "ASSIGNED Review");
}

static DataMap withAssignee(const std::string& who) {
    DataMap d;
    d["assignee"] = who;
    return d;
}

TEST_CASE("NotificationService only reacts when a person must act") {
    WorkflowInstance instance("I-5", 0, DataMap());
    FakeConnector sms;
    NotificationService notifier(&instance, &sms, "supervisor");

    instance.raiseEvent(EventType::STARTED, "Review");
    instance.raiseEvent(EventType::COMPLETED, "Review");
    CHECK(notifier.getSent().empty());
    CHECK(sms.sends == 0);

    instance.notifyObservers(WorkflowEvent(EventType::ASSIGNED, "Review", withAssignee("Anna")));
    instance.raiseEvent(EventType::ESCALATED, "Approve", 0, "no response in 2 days");
    instance.raiseEvent(EventType::REJECTED, "Approve");
    CHECK(notifier.getSent().size() == 3);
    CHECK(sms.sends == 3);
}

TEST_CASE("NotificationService sends in the Adapter's format") {
    WorkflowInstance instance("I-6", 0, DataMap());
    FakeConnector sms;
    NotificationService notifier(&instance, &sms, "supervisor");

    SUBCASE("assignee is the recipient") {
        instance.notifyObservers(WorkflowEvent(EventType::ASSIGNED, "Review", withAssignee("Anna")));
        CHECK(sms.lastTo == "Anna");
        CHECK(sms.lastMessage == "ASSIGNED: Review");
        CHECK(sms.lastUrgent == "false");
    }
    SUBCASE("explicit 'to' wins over assignee") {
        DataMap d = withAssignee("Anna");
        d["to"] = "anna@example.com";
        instance.notifyObservers(WorkflowEvent(EventType::ASSIGNED, "Review", d));
        CHECK(sms.lastTo == "anna@example.com");
    }
    SUBCASE("escalation with no assignee goes to the default recipient, urgent") {
        instance.raiseEvent(EventType::ESCALATED, "Approve", 0, "no response in 2 days");
        CHECK(sms.lastTo == "supervisor");
        CHECK(sms.lastUrgent == "true");
        CHECK(sms.lastMessage == "ESCALATED: Approve (no response in 2 days)");
    }
}

TEST_CASE("NotificationService does not send when nobody is named") {
    WorkflowInstance instance("I-7", 0, DataMap());
    FakeConnector sms;
    NotificationService notifier(&instance, &sms);
    instance.raiseEvent(EventType::REJECTED, "Approve");
    CHECK(notifier.getSent().size() == 1);
    CHECK(sms.sends == 0);
}

TEST_CASE("NotificationService works without a channel") {
    WorkflowInstance instance("I-8", 0, DataMap());
    NotificationService notifier(&instance);
    instance.raiseEvent(EventType::ASSIGNED, "Review");
    CHECK(notifier.getSent().size() == 1);
}

TEST_CASE("Notifications sent as commands are retried when the messaging service fails") {
    WorkflowInstance instance("I-N1", 0, DataMap());
    AuditLogger logger(&instance);
    IncidentMonitor monitor(&instance, instance.getProcessor(), 3);
    FakeConnector sms(2, true);
    NotificationService notifier(&instance, &sms, "supervisor", instance.getProcessor());

    instance.notifyObservers(WorkflowEvent(EventType::ASSIGNED, "Review", withAssignee("Anna")));

    CHECK(sms.sends == 1);
    CHECK(monitor.getIncidents().empty());
    REQUIRE(instance.getProcessor()->getHistory().size() == 1);
    CHECK(instance.getProcessor()->getHistory()[0]->getAttempts() == 3);
    CHECK(logger.getTrail().back().getType() == EventType::NOTIFIED);
    CHECK(logger.getTrail().back().get("command") == "SendNotification");
}

TEST_CASE("A notification that keeps failing raises one incident") {
    WorkflowInstance instance("I-N2", 0, DataMap());
    IncidentMonitor monitor(&instance, instance.getProcessor(), 3);
    FakeConnector sms(10, true);
    NotificationService notifier(&instance, &sms, "supervisor", instance.getProcessor());

    instance.raiseEvent(EventType::ESCALATED, "Approve");

    CHECK(sms.sends == 0);
    REQUIRE(monitor.getIncidents().size() == 1);
    CHECK(monitor.getIncidents()[0] == "INCIDENT Approve: retries exhausted (messaging unavailable)");
}

TEST_CASE("Compensation skips sent notifications") {
    WorkflowInstance instance("I-N3", 0, DataMap());
    FakeConnector sms;
    NotificationService notifier(&instance, &sms, "supervisor", instance.getProcessor());
    instance.notifyObservers(WorkflowEvent(EventType::ASSIGNED, "Review", withAssignee("Anna")));

    AuditLogger logger(&instance);
    instance.getProcessor()->compensate();

    CHECK(instance.getProcessor()->getHistory()[0]->getStatus() == "DONE");
    CHECK(logger.getTrail().empty());
}

TEST_CASE("Without a processor, failed sends are kept") {
    WorkflowInstance instance("I-N4", 0, DataMap());
    FakeConnector sms(1, true);
    NotificationService notifier(&instance, &sms, "supervisor");
    instance.raiseEvent(EventType::REJECTED, "Approve");
    REQUIRE(notifier.getFailed().size() == 1);
    CHECK(notifier.getFailed()[0] == "REJECTED: Approve");
}

TEST_CASE("IncidentMonitor raises an incident when a deadline passes") {
    WorkflowInstance instance("I-7", 0, DataMap());
    IncidentMonitor monitor(&instance, instance.getProcessor());
    instance.raiseEvent(EventType::TIMED_OUT, "Upload documents");
    REQUIRE(monitor.getIncidents().size() == 1);
    CHECK(monitor.getIncidents()[0] == "INCIDENT Upload documents: deadline passed");
}

TEST_CASE("IncidentMonitor raises an incident for a failure with no command") {
    WorkflowInstance instance("I-8", 0, DataMap());
    IncidentMonitor monitor(&instance, instance.getProcessor());
    instance.raiseEvent(EventType::FAILED, "Pay", 0, "unknown error");
    CHECK(monitor.getIncidents().size() == 1);
}

TEST_CASE("An observer may detach itself during notify") {
    struct OneShot : public WorkflowObserver {
        explicit OneShot(WorkflowSubject* s) : WorkflowObserver(s), seen(0) {}
        void update(const WorkflowEvent&) { ++seen; if (subject) subject->detach(this); subject = 0; }
        int seen;
    };

    WorkflowInstance instance("I-9", 0, DataMap());
    OneShot once(&instance);
    AuditLogger logger(&instance);

    instance.raiseEvent(EventType::STARTED);
    instance.raiseEvent(EventType::COMPLETED);

    CHECK(once.seen == 1);
    CHECK(logger.getTrail().size() == 2);
}
