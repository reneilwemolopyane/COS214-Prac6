/**
 * @file test_workflow_engine.cpp
 * @brief Unit tests for WorkflowEngine (Facade subsystem and Command pattern Client).
 */
#include "tests/doctest.h"
#include "tests/TestConnectors.h"
#include "command/WorkflowEngine.h"
#include "core/Participant.h"
#include "composite_prototype/WorkItem.h"
#include "observer/Observers.h"

TEST_CASE("Engine registers instances once") {
    WorkflowEngine engine;
    WorkflowInstance* a = new WorkflowInstance("A", 0, DataMap());

    CHECK(engine.registerInstance(a));
    CHECK(engine.getStatus("A") == "REGISTERED");
    CHECK(engine.findInstance("A") == a);

    WorkflowInstance duplicate("A", 0, DataMap());
    CHECK_FALSE(engine.registerInstance(&duplicate));
    CHECK_FALSE(engine.registerInstance(0));
    CHECK(engine.instanceCount() == 1);
}

TEST_CASE("Engine starts an instance once and announces it") {
    WorkflowEngine engine;
    WorkflowInstance* a = new WorkflowInstance("A", 0, DataMap());
    engine.registerInstance(a);
    AuditLogger logger(a);

    CHECK(engine.start("A"));
    CHECK(engine.getStatus("A") == "RUNNING");
    REQUIRE_FALSE(logger.getTrail().empty());
    CHECK(logger.getTrail().back().getType() == EVT_STARTED);

    CHECK_FALSE(engine.start("A"));
    CHECK_FALSE(engine.start("missing"));
    CHECK(engine.getStatus("missing") == "NOT_FOUND");
}

TEST_CASE("Engine only accepts work for running instances") {
    WorkflowEngine engine;
    WorkflowInstance* a = new WorkflowInstance("A", 0, DataMap());
    engine.registerInstance(a);
    WorkItem item("Review");
    Participant anna("p1", "Anna");

    SUBCASE("before start") {
        CHECK_FALSE(engine.assignWorkItem(a, &item, &anna));
        CHECK(a->getProcessor()->getHistory().empty());
    }
    SUBCASE("unregistered instance") {
        WorkflowInstance stranger("S", 0, DataMap());
        CHECK_FALSE(engine.completeWorkItem(&stranger, &item, "done"));
    }
    SUBCASE("after start") {
        engine.start(a);
        CHECK(engine.assignWorkItem(a, &item, &anna));
        CHECK(engine.completeWorkItem(a, &item, "approved"));
        CHECK(a->getProcessor()->getHistory().size() == 2);
    }
}

TEST_CASE("Engine cancel compensates the instance's completed work") {
    WorkflowEngine engine;
    WorkflowInstance* a = new WorkflowInstance("A", 0, DataMap());
    engine.registerInstance(a);
    engine.start(a);
    AuditLogger logger(a);
    FakeConnector bank;
    WorkItem item("Pay out");

    CHECK(engine.invokeService(a, &bank, payRequest(), "Pay out", "reverse"));
    CHECK(engine.completeWorkItem(a, &item, "paid"));

    CHECK(engine.cancel("A"));
    CHECK(bank.reversals == 1);
    CHECK(item.getOutcome().empty());
    CHECK(engine.getStatus("A") == "CANCELLED");
    CHECK(logger.getTrail().back().getType() == EVT_CANCELLED);

    CHECK_FALSE(engine.cancel("A"));
    CHECK_FALSE(engine.completeWorkItem(a, &item, "again"));
}

TEST_CASE("Instances are independent") {
    WorkflowEngine engine;
    WorkflowInstance* a = new WorkflowInstance("A", 0, DataMap());
    WorkflowInstance* b = new WorkflowInstance("B", 0, DataMap());
    engine.registerInstance(a);
    engine.registerInstance(b);
    engine.start("A");
    engine.start("B");
    WorkItem itemA("Task"), itemB("Task");

    engine.completeWorkItem(a, &itemA, "done");
    engine.completeWorkItem(b, &itemB, "done");
    engine.cancel("A");

    CHECK(itemA.getOutcome().empty());
    CHECK(itemB.getOutcome() == "done");
    CHECK(engine.getStatus("B") == "RUNNING");
}