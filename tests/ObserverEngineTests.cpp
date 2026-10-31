// Tests for the Observer pattern, the concrete observers and the engine wiring
// (builder -> definition -> instance -> engine). Uses the doctest main in test_main.cpp.
#include "doctest.h"
#include <string>
#include <vector>

#include "core/WorkflowInstance.h"
#include "core/Participant.h"
#include "observer/ObserverPattern.h"
#include "observer/Observers.h"
#include "command/Command.h"
#include "command/CommandProcessor.h"
#include "command/WorkflowEngine.h"
#include "builder/ValidatingWorkflowBuilder.h"
#include "facade/DefinitionRepository.h"
#include "facade/WorkflowFacade.h"
#include "composite_prototype/Stage.h"
#include "composite_prototype/WorkItem.h"
#include "adapter/PaymentAdapter.h"
#include "state/InvalidTransitionException.h"

namespace {

// ---------- helpers ----------

class Recorder : public WorkflowObserver {
public:
    void update(const WorkflowEvent& e) override { types.push_back(e.getType()); last = e; }
    int count(EventType t) const { int n = 0; for (size_t i = 0; i < types.size(); ++i) if (types[i] == t) ++n; return n; }
    std::vector<EventType> types;
    WorkflowEvent last;
};

// Detaches itself the first time it is called.
class OneShot : public WorkflowObserver {
public:
    OneShot(WorkflowSubject* s) : subject(s), calls(0) {}
    void update(const WorkflowEvent&) override { ++calls; subject->detach(this); }
    WorkflowSubject* subject;
    int calls;
};

// Fails `failures` times, then succeeds. Retryable or not.
class FlakyCommand : public WorkflowCommand {
public:
    FlakyCommand(int failures, bool retry) : WorkflowCommand("item1"), left(failures), retry(retry) {}
    bool execute() override {
        if (left > 0) { --left; error = "service down"; return false; }
        error.clear(); return true;
    }
    void undo() override {}
    std::string name() const override { return "Flaky"; }
    bool isRetryable() const override { return retry; }
private:
    int left; bool retry;
};

class FakeConnector : public ExternalConnector {
public:
    FakeConnector(bool ok) : ok(ok), calls(0) {}
    std::string name() const override { return "fake"; }
    ConnectorResult execute(const ConnectorRequest& r) override {
        ++calls; lastOperation = r.operation; lastTo = r.params.count("to") ? r.params.find("to")->second : "";
        return ConnectorResult(ok, false, ok ? "sent" : "refused");
    }
    bool ok; int calls; std::string lastOperation, lastTo;
};

WorkflowInstance* bareInstance() { return new WorkflowInstance("i1", new Stage("root"), DataMap()); }

// A claim-like definition: police and inspect run in parallel, decision waits for both, payout for decision.
struct Harness {
    DefinitionRepository repo;
    WorkflowEngine engine;
    WorkflowFacade facade;
    std::string firstId;

    Harness(const std::string& role = "Assessor", bool withParticipants = true) : facade(repo, engine) {
        ValidatingWorkflowBuilder b;
        b.setIdentity("claim", 1);
        b.buildStage("Assessment");
        b.buildWorkItem("police", role);
        b.buildWorkItem("inspect", role);
        b.buildWorkItem("decision", role);
        b.buildDependency("police", "decision");
        b.buildDependency("inspect", "decision");
        facade.registerDefinition(b.getResult());
        if (withParticipants) {
            Participant* a = new Participant("P1", "Thandi"); a->addRole("Assessor");
            Participant* c = new Participant("P2", "Sipho");  c->addRole("Assessor");
            facade.addParticipant(a);
            facade.addParticipant(c);
        }
    }
    WorkflowInstance* start() { firstId = facade.startWorkflow("claim", DataMap()); return engine.findInstance(firstId); }
    static WorkItem* item(WorkflowInstance* in, const std::string& id) { return in->getCoordinator()->findItem(id); }
    void finish(WorkflowInstance* in, const std::string& id) {
        WorkItem* w = item(in, id);
        w->start();
        engine.completeWorkItem(in, w, "ok");
    }
};

} // namespace

// ================= WorkflowSubject / WorkflowObserver =================

TEST_CASE("Observer: a type subscription receives only that type") {
    WorkflowSubject s; Recorder r;
    s.attach(EVT_COMPLETED, &r);
    s.notifyObservers(WorkflowEvent(EVT_ASSIGNED, 0, "a"));
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    CHECK_EQ(1, (int)r.types.size());
    CHECK_EQ(1, r.count(EVT_COMPLETED));
}

TEST_CASE("Observer: attach without a type receives every event") {
    WorkflowSubject s; Recorder r;
    s.attach(&r);
    s.notifyObservers(WorkflowEvent(EVT_ASSIGNED, 0, "a"));
    s.notifyObservers(WorkflowEvent(EVT_FAILED, 0, "a"));
    CHECK_EQ(2, (int)r.types.size());
}

TEST_CASE("Observer: attaching twice delivers once; null is ignored") {
    WorkflowSubject s; Recorder r;
    s.attach(EVT_COMPLETED, &r);
    s.attach(EVT_COMPLETED, &r);
    s.attach(EVT_COMPLETED, 0);
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    CHECK_EQ(1, (int)r.types.size());
    CHECK_EQ(1, (int)s.subscriptionCount());
}

TEST_CASE("Observer: detach stops delivery") {
    WorkflowSubject s; Recorder r;
    s.attach(&r);
    s.detach(&r);
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    CHECK(r.types.empty());
}

TEST_CASE("Observer: detaching one type keeps the others") {
    WorkflowSubject s; Recorder r;
    s.attach(EVT_COMPLETED, &r);
    s.attach(EVT_FAILED, &r);
    s.detach(EVT_COMPLETED, &r);
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    s.notifyObservers(WorkflowEvent(EVT_FAILED, 0, "a"));
    CHECK_EQ(0, r.count(EVT_COMPLETED));
    CHECK_EQ(1, r.count(EVT_FAILED));
}

TEST_CASE("Observer: an observer may detach itself while being notified") {
    WorkflowSubject s; OneShot once(&s); Recorder r;
    s.attach(&once);
    s.attach(&r);
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    CHECK_EQ(1, once.calls);
    CHECK_EQ(2, (int)r.types.size());   // the other observer is not skipped
}

TEST_CASE("Observer: the subject stamps a timestamp when the event has none") {
    WorkflowSubject s; Recorder r;
    s.attach(&r);
    s.notifyObservers(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    CHECK_FALSE(r.last.getTimestamp().empty());
}

TEST_CASE("Observer: an instance adds its own id to events") {
    WorkflowInstance* in = bareInstance();
    Recorder r; in->attach(&r);
    in->raiseEvent(EVT_STARTED);
    CHECK_EQ(std::string("i1"), r.last.getInstanceId());
    delete in;
}

// ================= AuditLogger / DependencyTracker =================

TEST_CASE("AuditLogger: every instance records every event in order") {
    WorkflowInstance* in = bareInstance();
    in->raiseEvent(EVT_STARTED);
    in->raiseEvent(EVT_COMPLETED, "a");
    CHECK_EQ(2, (int)in->getAuditLogger()->size());
    CHECK_EQ(EVT_STARTED, in->getAuditLogger()->getTrail()[0].getType());
    CHECK_EQ(EVT_COMPLETED, in->getAuditLogger()->getTrail()[1].getType());
    delete in;
}

TEST_CASE("AuditLogger: lines are readable and stored events keep no pointers") {
    WorkflowInstance* in = bareInstance();
    in->raiseEvent(EVT_FAILED, "a", 0, "boom");
    std::string line = in->getAuditLogger()->getLines()[0];
    CHECK(line.find("FAILED a") != std::string::npos);
    CHECK(line.find("boom") != std::string::npos);
    CHECK(in->getAuditLogger()->getTrail()[0].source == 0);
    delete in;
}

TEST_CASE("DependencyTracker: tracks completed and closed items without duplicates") {
    DependencyTracker t;
    t.update(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    t.update(WorkflowEvent(EVT_COMPLETED, 0, "a"));
    t.update(WorkflowEvent(EVT_REJECTED, 0, "b"));
    CHECK_EQ(1, (int)t.completedCount());
    CHECK_EQ(1, (int)t.closedCount());
    CHECK(t.isCompleted("a"));
    CHECK(t.isClosed("b"));
    CHECK_FALSE(t.isCompleted("b"));
}

// ================= IncidentMonitor =================

TEST_CASE("IncidentMonitor: retries a temporary failure until it succeeds") {
    WorkflowInstance* in = bareInstance();
    FlakyCommand* c = new FlakyCommand(2, true);
    CHECK(in->getProcessor()->run(c));
    CHECK_EQ(3, c->getAttempts());
    CHECK_EQ(std::string("DONE"), c->getStatus());
    CHECK(in->getIncidentMonitor()->getIncidents().empty());
    delete in;
}

TEST_CASE("IncidentMonitor: gives up after the maximum attempts and raises an incident") {
    WorkflowInstance* in = bareInstance();
    FlakyCommand* c = new FlakyCommand(10, true);
    CHECK_FALSE(in->getProcessor()->run(c));
    CHECK_EQ(3, c->getAttempts());
    CHECK_EQ(1, (int)in->getIncidentMonitor()->getIncidents().size());
    delete in;
}

TEST_CASE("IncidentMonitor: a permanent failure is not retried") {
    WorkflowInstance* in = bareInstance();
    FlakyCommand* c = new FlakyCommand(1, false);
    CHECK_FALSE(in->getProcessor()->run(c));
    CHECK_EQ(1, c->getAttempts());
    CHECK_EQ(1, (int)in->getIncidentMonitor()->getIncidents().size());
    delete in;
}

// ================= NotificationService =================

TEST_CASE("NotificationService: tells the assignee, through the connector when there is one") {
    FakeConnector conn(true);
    NotificationService n(&conn);
    n.update(WorkflowEvent(EVT_ASSIGNED, 0, "police", 0, "assignee: Thandi"));
    CHECK_EQ(1, (int)n.getMessages().size());
    CHECK_EQ(1, conn.calls);
    CHECK_EQ(std::string("send"), conn.lastOperation);
    CHECK_EQ(std::string("Thandi"), conn.lastTo);
}

TEST_CASE("NotificationService: works without a connector and counts delivery failures") {
    NotificationService plain;
    plain.update(WorkflowEvent(EVT_ASSIGNED, 0, "a", 0, "assignee: Sipho"));
    CHECK_EQ(1, (int)plain.getMessages().size());

    FakeConnector refusing(false);
    NotificationService n(&refusing);
    n.update(WorkflowEvent(EVT_ESCALATED, 0, "a", 0, "no one available"));
    CHECK_EQ(1, n.getFailureCount());
    CHECK_EQ(std::string("supervisor"), refusing.lastTo);
}

// ================= Builder, states =================

TEST_CASE("Builder: a work item keeps its name as id and gets the role") {
    ValidatingWorkflowBuilder b;
    b.setIdentity("d", 1);
    b.buildStage("S");
    b.buildWorkItem("police", "Assessor");
    WorkflowDefinition* d = b.getResult();
    WorkflowInstance* in = d->createInstance(DataMap());
    in->start();
    WorkItem* w = in->getCoordinator()->findItem("police");
    REQUIRE(w != 0);
    CHECK_EQ(std::string("police"), w->getName());
    CHECK_EQ(std::string("Assessor"), w->getRole());
    delete in;
    delete d;
}

TEST_CASE("State: a new item is pending, a released item is available") {
    WorkItem w("a", "A");
    CHECK(w.isPending());
    CHECK_FALSE(w.isAvailable());
    w.makeAvailable();
    CHECK_FALSE(w.isPending());
    CHECK(w.isAvailable());
}

// ================= Engine end to end =================

TEST_CASE("Engine: independent work is assigned at start, dependent work waits") {
    Harness h; WorkflowInstance* in = h.start();
    CHECK_EQ(std::string("RUNNING"), h.facade.getInstanceStatus(h.firstId));
    CHECK_EQ(std::string("Assigned"), Harness::item(in, "police")->getStateName());
    CHECK_EQ(std::string("Assigned"), Harness::item(in, "inspect")->getStateName());
    CHECK(Harness::item(in, "decision")->isPending());
    CHECK(Harness::item(in, "police")->getAssignee() != 0);
}

TEST_CASE("Engine: a join releases only after every prerequisite completes") {
    Harness h; WorkflowInstance* in = h.start();
    h.finish(in, "police");
    CHECK(Harness::item(in, "decision")->isPending());
    h.finish(in, "inspect");
    CHECK_EQ(std::string("Assigned"), Harness::item(in, "decision")->getStateName());
    h.finish(in, "decision");
    CHECK(in->isFinished());
    CHECK(in->isCompleted());
}

TEST_CASE("Engine: an action the state forbids is refused, explained and audited (no crash)") {
    Harness h; WorkflowInstance* in = h.start();
    CHECK_FALSE(h.engine.completeWorkItem(in, Harness::item(in, "police"), "x"));   // Assigned, not started
    CHECK(h.engine.getLastError().find("complete") != std::string::npos);
    CHECK_EQ(std::string("Assigned"), Harness::item(in, "police")->getStateName());   // nothing changed
    bool refused = false;
    const std::vector<WorkflowEvent>& trail = in->getAuditLogger()->getTrail();
    for (std::size_t i = 0; i < trail.size(); ++i) if (trail[i].type == EVT_REFUSED) refused = true;
    CHECK(refused);
}

TEST_CASE("Engine: no eligible participant escalates the item and raises FAILED") {
    Harness h("Surgeon", true);
    WorkflowInstance* in = h.start();
    CHECK_EQ(std::string("Escalated"), Harness::item(in, "police")->getStateName());
    int failed = 0;
    for (size_t i = 0; i < in->getAuditLogger()->getTrail().size(); ++i)
        if (in->getAuditLogger()->getTrail()[i].getType() == EVT_FAILED) ++failed;
    CHECK_GE(failed, 1);
}

TEST_CASE("Engine: instances from one definition are independent") {
    Harness h;
    WorkflowInstance* a = h.start();
    std::string idB = h.facade.startWorkflow("claim", DataMap());
    WorkflowInstance* b = h.engine.findInstance(idB);
    REQUIRE(a != b);
    h.finish(a, "police");
    h.finish(a, "inspect");
    CHECK_EQ(std::string("Assigned"), Harness::item(a, "decision")->getStateName());
    CHECK(Harness::item(b, "decision")->isPending());
    CHECK_EQ(0, b->getDependencyTracker()->completedCount());
}

TEST_CASE("Engine: each completion appears once in the audit trail") {
    Harness h; WorkflowInstance* in = h.start();
    h.finish(in, "police");
    int completed = 0;
    for (size_t i = 0; i < in->getAuditLogger()->getTrail().size(); ++i)
        if (in->getAuditLogger()->getTrail()[i].getType() == EVT_COMPLETED) ++completed;
    CHECK_EQ(1, completed);
}

TEST_CASE("Engine: a service call is announced as SERVICE_CALLED, not as a completed work item") {
    Harness h; WorkflowInstance* in = h.start();
    PaymentAPI api; PaymentAdapter pay("pay", &api);
    ConnectorRequest r; r.operation = "pay"; r.params["amount"] = "5000"; r.params["account"] = "A1";
    CHECK(h.engine.invokeService(in, &pay, r, "police"));
    CHECK_EQ(0, (int)in->getDependencyTracker()->completedCount());
    CHECK_EQ(EVT_SERVICE_CALLED, in->getAuditLogger()->getTrail().back().getType());
}

TEST_CASE("Engine: cancelling refunds the payment, cancels open items and finishes the instance") {
    Harness h; WorkflowInstance* in = h.start();
    PaymentAPI api; PaymentAdapter pay("pay", &api);
    ConnectorRequest r; r.operation = "pay"; r.params["amount"] = "5000"; r.params["account"] = "A1";
    REQUIRE(h.engine.invokeService(in, &pay, r, "police"));
    REQUIRE(api.knows("TXN-1"));
    h.finish(in, "police");

    CHECK(h.engine.cancel(h.firstId));

    CHECK(api.knows("TXN-1"));
    CHECK_FALSE(api.reverse("TXN-1"));   // already reversed by the undo
    CHECK_EQ(std::string("CANCELLED"), h.facade.getInstanceStatus(h.firstId));
    CHECK_EQ(std::string("Cancelled"), Harness::item(in, "police")->getStateName());
    CHECK_EQ(std::string("Cancelled"), Harness::item(in, "decision")->getStateName());
    CHECK(in->isFinished());
    CHECK_FALSE(in->isCompleted());
}

TEST_CASE("Engine: participants are shared by the engine and released when work ends") {
    Harness h; WorkflowInstance* in = h.start();
    CHECK_EQ(2, (int)h.engine.getParticipants().size());
    int total = h.engine.getParticipants()[0]->getWorkload() + h.engine.getParticipants()[1]->getWorkload();
    CHECK_EQ(2, total);                  // police and inspect
    h.finish(in, "police");
    total = h.engine.getParticipants()[0]->getWorkload() + h.engine.getParticipants()[1]->getWorkload();
    CHECK_EQ(1, total);
}
