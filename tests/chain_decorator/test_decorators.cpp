#include "doctest/doctest.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "StubItem.h"
#include "Decorators.h"
#include "WorkItemDecorator.h"

typedef std::vector<std::string> Log;

// ---------- base forwarding ----------
TEST_CASE("a decorator forwards calls to the wrapped item") {
    Log log;
    StubItem* raw = new StubItem("Task", &log);
    ValidationDecorator d(raw);   // no rule: just forwards

    d.start();
    d.complete();
    d.cancel();

    CHECK(d.getName() == "Task");
    CHECK(d.isComplete());
    REQUIRE(log.size() == 3);
    CHECK(log[0] == "start:Task");
    CHECK(log[1] == "complete:Task");
    CHECK(log[2] == "cancel:Task");
}

// ---------- validation ----------
TEST_CASE("validation blocks completion when the rule fails") {
    Log log;
    ValidationDecorator d(new StubItem("Task", &log), [] { return false; });

    CHECK_THROWS_AS(d.complete(), std::runtime_error);
    CHECK_FALSE(d.isComplete());
    CHECK(log.empty());   // wrapped item was never touched
}

TEST_CASE("validation allows completion when the rule passes") {
    Log log;
    ValidationDecorator d(new StubItem("Task", &log), [] { return true; });

    d.complete();

    CHECK(d.isComplete());
}

TEST_CASE("validation rule is checked each time, so it can change") {
    Log log;
    bool documentsReceived = false;
    ValidationDecorator d(new StubItem("Task", &log),
                          [&documentsReceived] { return documentsReceived; });

    CHECK_THROWS_AS(d.complete(), std::runtime_error);
    documentsReceived = true;
    d.complete();

    CHECK(d.isComplete());
}

// ---------- priority ----------
TEST_CASE("priority keeps getName() clean and offers a display name") {
    Log log;
    PriorityDecorator p(new StubItem("Inspect", &log), 3);

    CHECK(p.getName() == "Inspect");
    CHECK(p.displayName() == "[P3] Inspect");
    CHECK(p.getPriority() == 3);
}

TEST_CASE("priority shortens deadlines and never divides by zero") {
    Log log;
    PriorityDecorator high(new StubItem("A", &log), 4);
    PriorityDecorator zero(new StubItem("B", &log), 0);
    PriorityDecorator negative(new StubItem("C", &log), -5);

    CHECK(high.adjustDeadline(100) == 25);
    CHECK(zero.adjustDeadline(100) == 100);
    CHECK(negative.adjustDeadline(100) == 100);
}

// ---------- security ----------
TEST_CASE("security denies the wrong role and allows the right one") {
    Log log;
    SecurityDecorator s(new StubItem("Approve", &log), "Manager", "Intern");

    CHECK_THROWS_AS(s.start(), std::runtime_error);
    CHECK_THROWS_AS(s.complete(), std::runtime_error);
    CHECK(log.empty());

    s.setActor("Manager");
    s.start();
    s.complete();
    CHECK(s.isComplete());
}

TEST_CASE("security does not block a system cancel") {
    Log log;
    SecurityDecorator s(new StubItem("Approve", &log), "Manager");   // no actor set
    CHECK_NOTHROW(s.cancel());
    CHECK(log.size() == 1);
}

TEST_CASE("security denies an empty actor") {
    Log log;
    SecurityDecorator s(new StubItem("Approve", &log), "Manager");

    CHECK_THROWS_AS(s.complete(), std::runtime_error);
}

TEST_CASE("security asks the actor provider every time") {
    Log log;
    std::string current = "Intern";
    SecurityDecorator s(new StubItem("Approve", &log), "Manager");
    s.setActorProvider([&current] { return current; });

    CHECK_THROWS_AS(s.start(), std::runtime_error);
    current = "Manager";
    s.start();
    CHECK(log.size() == 1);
}

TEST_CASE("a cloned security decorator keeps its provider") {
    Log log;
    std::string current = "Manager";
    SecurityDecorator s(new StubItem("Approve", &log), "Manager");
    s.setActorProvider([&current] { return current; });

    std::unique_ptr<WorkflowComponent> copy(s.clone());
    current = "Intern";

    CHECK_THROWS_AS(copy->complete(), std::runtime_error);
}

// ---------- audit ----------
TEST_CASE("audit records each lifecycle call") {
    Log log;
    AuditDecorator a(new StubItem("Task", &log));

    a.start();
    a.complete();
    a.cancel();

    const std::vector<std::string>& h = a.history();
    REQUIRE(h.size() == 3);
    CHECK(h[0] == "start: Task");
    CHECK(h[1] == "complete: Task");
    CHECK(h[2] == "cancel: Task");
}

TEST_CASE("audit also records a FAILED entry when the wrapped call throws") {
    Log log;
    AuditDecorator a(new ValidationDecorator(new StubItem("Check", &log),
                                             [] { return false; }));

    CHECK_THROWS_AS(a.complete(), std::runtime_error);

    const std::vector<std::string>& h = a.history();
    REQUIRE(h.size() == 2);
    CHECK(h[0] == "complete: Check");
    CHECK(h[1] == "complete FAILED: Check");
}

TEST_CASE("audit passes every entry to its sink") {
    Log log;
    std::vector<std::string> seen;
    AuditDecorator a(new StubItem("Task", &log),
                     [&seen](const std::string& s) { seen.push_back(s); });

    a.start();
    a.complete();

    REQUIRE(seen.size() == 2);
    CHECK(seen[0] == "start: Task");
    CHECK(seen[1] == "complete: Task");
}

TEST_CASE("a cloned audit decorator starts with an empty trail") {
    Log log;
    AuditDecorator a(new StubItem("Task", &log));
    a.start();

    std::unique_ptr<WorkflowComponent> copy(a.clone());
    AuditDecorator* copyAudit = dynamic_cast<AuditDecorator*>(copy.get());
    REQUIRE(copyAudit != 0);

    CHECK(copyAudit->history().empty());
    copy->start();
    CHECK(copyAudit->history().size() == 1);
    CHECK(a.history().size() == 1);   // original not affected
}

// ---------- notification ----------
TEST_CASE("notification sends a message after each successful action") {
    Log log;
    std::vector<std::string> msgs;
    NotificationDecorator n(new StubItem("Task", &log),
                            [&msgs](const std::string& s) { msgs.push_back(s); });

    n.start();
    n.complete();

    REQUIRE(msgs.size() == 2);
    CHECK(msgs[0] == "started: Task");
    CHECK(msgs[1] == "completed: Task");
}

TEST_CASE("notification stays quiet when the wrapped action fails") {
    Log log;
    std::vector<std::string> msgs;
    NotificationDecorator n(
        new ValidationDecorator(new StubItem("Task", &log), [] { return false; }),
        [&msgs](const std::string& s) { msgs.push_back(s); });

    CHECK_THROWS_AS(n.complete(), std::runtime_error);
    CHECK(msgs.empty());
}

// ---------- deadline ----------
TEST_CASE("deadline is not overdue before the item has started") {
    Log log;
    long now = 1000;
    DeadlineDecorator d(new StubItem("Task", &log), 60, [&now] { return now; });

    now = 999999;
    CHECK_FALSE(d.isOverdue());
    CHECK(d.remaining() == 0);
}

TEST_CASE("deadline becomes overdue only after the limit has passed") {
    Log log;
    long now = 1000;
    DeadlineDecorator d(new StubItem("Task", &log), 60, [&now] { return now; });

    d.start();
    CHECK_FALSE(d.isOverdue());
    CHECK(d.remaining() == 60);

    now = 1030;
    CHECK_FALSE(d.isOverdue());
    CHECK(d.remaining() == 30);

    now = 1060;                      // exactly at the limit: not overdue yet
    CHECK_FALSE(d.isOverdue());

    now = 1061;
    CHECK(d.isOverdue());
    CHECK(d.remaining() == 0);
}

TEST_CASE("a cloned deadline decorator has fresh timing") {
    Log log;
    long now = 1000;
    DeadlineDecorator d(new StubItem("Task", &log), 60, [&now] { return now; });
    d.start();
    now = 2000;
    REQUIRE(d.isOverdue());

    std::unique_ptr<WorkflowComponent> copy(d.clone());
    DeadlineDecorator* copyDeadline = dynamic_cast<DeadlineDecorator*>(copy.get());
    REQUIRE(copyDeadline != 0);

    CHECK_FALSE(copyDeadline->isOverdue());   // not started yet
}

// ---------- stacking and cloning ----------
TEST_CASE("decorators can be stacked and each layer does its job") {
    Log log;
    std::vector<std::string> msgs;
    std::string current = "Manager";

    SecurityDecorator* security = new SecurityDecorator(
        new AuditDecorator(
            new NotificationDecorator(
                new StubItem("Authorise", &log),
                [&msgs](const std::string& s) { msgs.push_back(s); })),
        "Manager");
    security->setActorProvider([&current] { return current; });
    std::unique_ptr<WorkflowComponent> item(security);

    item->start();
    item->complete();

    CHECK(item->isComplete());
    CHECK(msgs.size() == 2);

    current = "Intern";
    CHECK_THROWS_AS(item->start(), std::runtime_error);   // security is outermost
}

TEST_CASE("cloning a stack gives an independent copy") {
    Log log;
    AuditDecorator original(
        new ValidationDecorator(new StubItem("Task", &log), [] { return true; }));

    std::unique_ptr<WorkflowComponent> copy(original.clone());
    copy->complete();

    CHECK(copy->isComplete());
    CHECK_FALSE(original.isComplete());   // completing the copy leaves the original alone
    CHECK(original.history().empty());
}

namespace {
/** Parent that records which child reported finishing. */
struct ProbeParent : StubItem {
    std::vector<WorkflowComponent*> heard;
    ProbeParent() : StubItem("Probe", 0) {}
    void childFinished(WorkflowComponent& c) override { heard.push_back(&c); }
};
}

TEST_CASE("a decorator forwards activate, isFinished and isFailed") {
    Log log;
    ValidationDecorator d(new StubItem("Task", &log));

    d.activate();
    REQUIRE(log.size() == 1);
    CHECK(log[0] == "activate:Task");
    CHECK_FALSE(d.isFinished());
    CHECK_FALSE(d.isFailed());

    d.complete();
    CHECK(d.isFinished());
}

TEST_CASE("a decorated item still tells its parent when it finishes") {
    Log log;
    ProbeParent parent;
    ValidationDecorator d(new StubItem("Task", &log));
    d.setParent(&parent);

    d.complete();

    REQUIRE(parent.heard.size() == 1);
    CHECK(parent.heard[0] == &d);   // parent sees the decorator, not the inner item
}

TEST_CASE("every layer of a stack relays the finish notification once") {
    Log log;
    ProbeParent parent;
    AuditDecorator d(new NotificationDecorator(new StubItem("Task", &log),
                                               [](const std::string&) {}));
    d.setParent(&parent);

    d.complete();

    CHECK(parent.heard.size() == 1);
}

TEST_CASE("a blocked completion does not notify the parent") {
    Log log;
    ProbeParent parent;
    ValidationDecorator d(new StubItem("Task", &log), [] { return false; });
    d.setParent(&parent);

    CHECK_THROWS_AS(d.complete(), std::runtime_error);
    CHECK(parent.heard.empty());
}

// ---------- command-style calls: assign(Participant*) and complete(outcome) ----------
TEST_CASE("security also guards assign and complete(outcome)") {
    Log log;
    SecurityDecorator s(new StubItem("Approve", &log), "Manager", "Intern");

    CHECK_THROWS_AS(s.assign(nullptr), std::runtime_error);
    CHECK_THROWS_AS(s.complete("approved"), std::runtime_error);
    CHECK(log.empty());

    s.setActor("Manager");
    s.assign(nullptr);
    s.complete("approved");

    REQUIRE(log.size() == 2);
    CHECK(log[0] == "assign:Approve");
    CHECK(log[1] == "complete(approved):Approve");
    CHECK(s.isComplete());
}

TEST_CASE("validation also blocks complete(outcome)") {
    Log log;
    bool ok = false;
    ValidationDecorator d(new StubItem("Task", &log), [&ok] { return ok; });

    CHECK_THROWS_AS(d.complete("approved"), std::runtime_error);
    CHECK(log.empty());

    ok = true;
    d.complete("approved");
    CHECK(d.isComplete());
}

TEST_CASE("audit records assign and complete(outcome), including failures") {
    Log log;
    AuditDecorator a(new ValidationDecorator(new StubItem("Task", &log),
                                             [] { return false; }));

    a.assign(nullptr);
    CHECK_THROWS_AS(a.complete("approved"), std::runtime_error);

    const std::vector<std::string>& h = a.history();
    REQUIRE(h.size() == 3);
    CHECK(h[0] == "assign: Task");
    CHECK(h[1] == "complete(approved): Task");
    CHECK(h[2] == "complete(approved) FAILED: Task");
}

TEST_CASE("notification announces complete(outcome)") {
    Log log;
    std::vector<std::string> msgs;
    NotificationDecorator n(new StubItem("Task", &log),
                            [&msgs](const std::string& s) { msgs.push_back(s); });

    n.complete("approved");

    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0] == "completed (approved): Task");
}

TEST_CASE("commands acting through a WorkflowComponent pointer pass every layer") {
    Log log;
    std::string current = "Intern";
    AuditDecorator* audit = new AuditDecorator(new StubItem("Task", &log));
    SecurityDecorator* security = new SecurityDecorator(audit, "Manager");
    security->setActorProvider([&current] { return current; });
    std::unique_ptr<WorkflowComponent> item(security);
    WorkflowComponent* target = item.get();   // what a command holds

    CHECK_THROWS_AS(target->assign(nullptr), std::runtime_error);
    CHECK_THROWS_AS(target->complete("approved"), std::runtime_error);
    CHECK(log.empty());
    CHECK(audit->history().empty());          // security blocked before audit recorded

    current = "Manager";
    target->assign(nullptr);
    target->complete("approved");

    CHECK(item->isComplete());
    REQUIRE(audit->history().size() == 2);
    CHECK(audit->history()[0] == "assign: Task");
    CHECK(audit->history()[1] == "complete(approved): Task");
}