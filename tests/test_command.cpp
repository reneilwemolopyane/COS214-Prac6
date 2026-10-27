/**
 * @file test_command.cpp
 * @brief Unit tests for the Command pattern (concrete commands and CommandProcessor),
 *        including how it works with the Observer pattern for retries and incidents.
 */
#include "tests/doctest.h"
#include "tests/TestConnectors.h"
#include "core/WorkflowInstance.h"
#include "core/Participant.h"
#include "composite_prototype/WorkItem.h"
#include "command/Command.h"
#include "command/CommandProcessor.h"
#include "observer/Observers.h"

TEST_CASE("AssignWorkItemCommand assigns and undo restores the previous assignee") {
    WorkItem item("Review");
    Participant anna("p1", "Anna");
    Participant ben("p2", "Ben");

    AssignWorkItemCommand first(&item, &anna);
    AssignWorkItemCommand second(&item, &ben);

    CHECK(first.execute());
    CHECK(second.execute());
    CHECK(item.getAssignee() == &ben);

    second.undo();
    CHECK(item.getAssignee() == &anna);
    first.undo();
    CHECK(item.getAssignee() == 0);
}

TEST_CASE("AssignWorkItemCommand fails safely with missing input") {
    WorkItem item("Review");
    AssignWorkItemCommand noPerson(&item, 0);
    CHECK_FALSE(noPerson.execute());
    CHECK(noPerson.getError() == "missing work item or participant");
}

TEST_CASE("CompleteWorkItemCommand completes and undo reopens") {
    WorkItem item("Inspect");
    CompleteWorkItemCommand cmd(&item, "approved");

    CHECK(cmd.execute());
    CHECK(item.getOutcome() == "approved");
    CHECK(cmd.successEvent() == EVT_COMPLETED);

    cmd.undo();
    CHECK(item.getOutcome().empty());
}

TEST_CASE("InvokeServiceCommand keeps the result data and reverses with it") {
    FakeConnector bank;
    InvokeServiceCommand pay(&bank, payRequest(), "Pay", "reverse");

    REQUIRE(pay.execute());
    CHECK(pay.getResultData().find("reference")->second == "REF-001");

    pay.undo();
    CHECK(bank.reversals == 1);
    CHECK(bank.lastReversed == "REF-001");
}

TEST_CASE("InvokeServiceCommand builds the undo from each operation's own result") {
    SUBCASE("book -> cancel with bookingRef") {
        DataMap out; out["bookingRef"] = "BK-7";
        RecordingConnector scheduler(out);
        ConnectorRequest book; book.operation = "book"; book.params["slot"] = "09:00";
        InvokeServiceCommand cmd(&scheduler, book, "Inspection", "cancel");

        REQUIRE(cmd.execute());
        cmd.undo();
        CHECK(scheduler.last.operation == "cancel");
        CHECK(scheduler.last.params["bookingRef"] == "BK-7");
        CHECK(scheduler.last.params["slot"] == "09:00");
    }
    SUBCASE("store -> delete with documentId") {
        DataMap out; out["documentId"] = "DOC-3";
        RecordingConnector storage(out);
        ConnectorRequest store; store.operation = "store";
        InvokeServiceCommand cmd(&storage, store, "Upload", "delete");

        REQUIRE(cmd.execute());
        cmd.undo();
        CHECK(storage.last.operation == "delete");
        CHECK(storage.last.params["documentId"] == "DOC-3");
    }
    SUBCASE("no undo operation -> not reversible, undo sends nothing") {
        DataMap out; out["id"] = "X";
        RecordingConnector service(out);
        ConnectorRequest send; send.operation = "notify";
        InvokeServiceCommand cmd(&service, send);

        CHECK_FALSE(cmd.isReversible());
        REQUIRE(cmd.execute());
        cmd.undo();
        CHECK(service.last.operation == "notify");
    }
}

TEST_CASE("InvokeServiceCommand reports whether a failure is retryable") {
    SUBCASE("temporary failure") {
        FakeConnector bank(1, true);
        InvokeServiceCommand pay(&bank, payRequest());
        CHECK_FALSE(pay.execute());
        CHECK(pay.isRetryable());
        CHECK(pay.getError() == "service unavailable");
    }
    SUBCASE("business error") {
        FakeConnector bank(1, false);
        InvokeServiceCommand pay(&bank, payRequest());
        CHECK_FALSE(pay.execute());
        CHECK_FALSE(pay.isRetryable());
    }
    SUBCASE("no connector") {
        InvokeServiceCommand pay(0, payRequest());
        CHECK_FALSE(pay.execute());
        CHECK_FALSE(pay.isRetryable());
    }
}

TEST_CASE("CommandProcessor records commands and reports results as events") {
    WorkflowInstance instance("I-1", 0, DataMap());
    AuditLogger logger(&instance);
    WorkItem item("Review");
    Participant anna("p1", "Anna");

    CommandProcessor* processor = instance.getProcessor();
    CHECK(processor->run(new AssignWorkItemCommand(&item, &anna)));
    CHECK(processor->run(new CompleteWorkItemCommand(&item, "approved")));

    CHECK(processor->getHistory().size() == 2);
    CHECK(processor->getHistory()[0]->getStatus() == "DONE");
    REQUIRE(logger.getTrail().size() == 2);
    CHECK(logger.getTrail()[0].getType() == EVT_ASSIGNED);
    CHECK(logger.getTrail()[1].getType() == EVT_COMPLETED);
    CHECK(logger.getTrail()[1].get("command") == "CompleteWorkItem");
}

TEST_CASE("CommandProcessor::compensate undoes completed commands newest first") {
    WorkflowInstance instance("I-2", 0, DataMap());
    AuditLogger logger(&instance);
    FakeConnector bank;
    WorkItem item("Pay out");
    Participant anna("p1", "Anna");

    CommandProcessor* processor = instance.getProcessor();
    processor->run(new AssignWorkItemCommand(&item, &anna));
    processor->run(new InvokeServiceCommand(&bank, payRequest(), "Pay out", "reverse"));
    processor->run(new CompleteWorkItemCommand(&item, "paid"));

    processor->compensate();

    CHECK(item.getOutcome().empty());
    CHECK(bank.reversals == 1);
    CHECK(item.getAssignee() == 0);
    for (std::size_t i = 0; i < processor->getHistory().size(); ++i)
        CHECK(processor->getHistory()[i]->getStatus() == "UNDONE");

    SUBCASE("each undo is announced, newest first") {
        const std::vector<WorkflowEvent>& t = logger.getTrail();
        REQUIRE(t.size() == 6);
        CHECK(t[3].getType() == EVT_COMPENSATED);
        CHECK(t[3].get("command") == "CompleteWorkItem");
        CHECK(t[5].get("command") == "AssignWorkItem");
    }
}

TEST_CASE("CommandProcessor::compensate escalates commands that cannot be undone") {
    WorkflowInstance instance("I-7", 0, DataMap());
    AuditLogger logger(&instance);
    FakeConnector sms;
    ConnectorRequest text; text.operation = "sendSms";

    InvokeServiceCommand* cmd = new InvokeServiceCommand(&sms, text, "Notify");
    instance.getProcessor()->run(cmd);
    instance.getProcessor()->compensate();

    CHECK(cmd->getStatus() == "DONE");
    CHECK(sms.reversals == 0);
    CHECK(logger.getTrail().back().getType() == EVT_ESCALATED);
    CHECK(logger.getTrail().back().get("message") == "cannot be undone automatically");
}

TEST_CASE("CommandProcessor only retries failed commands it owns") {
    WorkflowInstance instance("I-3", 0, DataMap());
    CommandProcessor* processor = instance.getProcessor();
    WorkItem item("Review");

    WorkflowCommand* done = new CompleteWorkItemCommand(&item, "ok");
    processor->run(done);
    CHECK_FALSE(processor->retry(done));

    CompleteWorkItemCommand stranger(&item, "ok");
    CHECK_FALSE(processor->retry(&stranger));
    CHECK_FALSE(processor->run(0));
}

TEST_CASE("Command + Observer: a temporary failure is retried until it succeeds") {
    WorkflowInstance instance("I-4", 0, DataMap());
    IncidentMonitor monitor(&instance, instance.getProcessor(), 3);
    FakeConnector bank(2, true);

    InvokeServiceCommand* pay = new InvokeServiceCommand(&bank, payRequest(), "Pay");
    CHECK(instance.getProcessor()->run(pay));
    CHECK(pay->getAttempts() == 3);
    CHECK(pay->getStatus() == "DONE");
    CHECK(monitor.getIncidents().empty());
}

TEST_CASE("Command + Observer: retries stop at the limit and raise one incident") {
    WorkflowInstance instance("I-5", 0, DataMap());
    IncidentMonitor monitor(&instance, instance.getProcessor(), 3);
    FakeConnector bank(10, true);

    InvokeServiceCommand* pay = new InvokeServiceCommand(&bank, payRequest(), "Pay");
    CHECK_FALSE(instance.getProcessor()->run(pay));
    CHECK(pay->getAttempts() == 3);
    REQUIRE(monitor.getIncidents().size() == 1);
    CHECK(monitor.getIncidents()[0] == "INCIDENT Pay: retries exhausted (service unavailable)");
}

TEST_CASE("Command + Observer: a business error is not retried") {
    WorkflowInstance instance("I-6", 0, DataMap());
    IncidentMonitor monitor(&instance, instance.getProcessor(), 3);
    FakeConnector bank(1, false);

    InvokeServiceCommand* pay = new InvokeServiceCommand(&bank, payRequest(), "Pay", "reverse");
    CHECK_FALSE(instance.getProcessor()->run(pay));
    CHECK(pay->getAttempts() == 1);
    CHECK(monitor.getIncidents().size() == 1);

    SUBCASE("failed commands are not undone") {
        instance.getProcessor()->compensate();
        CHECK(bank.reversals == 0);
    }
}