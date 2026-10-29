#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <string>
#include <vector>
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Participant.h"
#include "tividy/state/States.h"
#include "tividy/strategy/AssignmentStrategy.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/mediator/InstanceCoordinator.h"

static Participant person(const std::string& id, const std::string& role) {
    Participant p(id, id);
    p.addRole(role);
    return p;
}

// ===================== STATE =====================
#define OP(n, call) static void op_##n(WorkItem& i) { i.call; }
OP(makeAvailable, makeAvailable())  OP(assign, assign())
OP(start, start())                  OP(complete, complete())
OP(reject, reject("x"))             OP(cancel, cancel())
OP(escalate, escalate("x"))         OP(suspend, suspend())
OP(resume, resume())                OP(reopen, reopen())

struct Op { const char* name; void (*fn)(WorkItem&); };
static const Op OPS[] = {
    {"makeAvailable", op_makeAvailable}, {"assign", op_assign}, {"start", op_start},
    {"complete", op_complete}, {"reject", op_reject}, {"cancel", op_cancel},
    {"escalate", op_escalate}, {"suspend", op_suspend}, {"resume", op_resume},
    {"reopen", op_reopen}
};

// every operation NOT in 'legal' must throw
static void checkOnlyAllowed(WorkItem& item, const std::string& legal) {
    for (size_t k = 0; k < sizeof(OPS) / sizeof(OPS[0]); k++) {
        if (legal.find(std::string(" ") + OPS[k].name + " ") != std::string::npos) continue;
        INFO("state " << item.getStateName() << ", operation " << OPS[k].name);
        CHECK_THROWS_AS(OPS[k].fn(item), InvalidTransitionException);
    }
}

TEST_CASE("State: happy path") {
    WorkItem item("a1", "review");
    Participant p = person("p", "R");
    CHECK(item.getStateName() == "Created");
    item.makeAvailable(); CHECK(item.getStateName() == "Available");
    item.assign(&p);      CHECK(item.getStateName() == "Assigned");
    item.start();         CHECK(item.getStateName() == "InProgress");
    item.complete();      CHECK(item.getStateName() == "Completed");
}

TEST_CASE("State: each state refuses everything except its legal moves") {
    WorkItem item("s1", "step");
    Participant p = person("p", "R");
    SUBCASE("Created")    { checkOnlyAllowed(item, " makeAvailable cancel "); }
    SUBCASE("Available")  { item.makeAvailable(); checkOnlyAllowed(item, " assign cancel "); }
    SUBCASE("Assigned")   { item.makeAvailable(); item.assign(&p);
                            checkOnlyAllowed(item, " assign start cancel escalate "); }
    SUBCASE("InProgress") { item.makeAvailable(); item.assign(&p); item.start();
                            checkOnlyAllowed(item, " complete reject cancel suspend "); }
    SUBCASE("Suspended")  { item.makeAvailable(); item.assign(&p); item.start(); item.suspend();
                            checkOnlyAllowed(item, " cancel resume "); }
    SUBCASE("Escalated")  { item.makeAvailable(); item.assign(&p); item.escalate("late");
                            checkOnlyAllowed(item, " start complete cancel "); }
    SUBCASE("Completed")  { item.makeAvailable(); item.assign(&p); item.start(); item.complete();
                            checkOnlyAllowed(item, " reopen "); }
    SUBCASE("Rejected")   { item.makeAvailable(); item.assign(&p); item.start(); item.reject("no");
                            checkOnlyAllowed(item, " "); }
    SUBCASE("Cancelled")  { item.cancel(); checkOnlyAllowed(item, " "); }
}

TEST_CASE("State: the exception says what failed") {
    WorkItem item("e1", "review");
    bool thrown = false;
    try { item.start(); }
    catch (const InvalidTransitionException& e) {
        thrown = true;
        CHECK(e.operation() == "start");
        CHECK(e.stateName() == "Created");
    }
    CHECK(thrown);
}

TEST_CASE("State: suspend, resume and reopen") {
    WorkItem item("r1", "review");
    Participant p = person("p", "R");
    item.makeAvailable(); item.assign(&p); item.start();
    item.suspend(); CHECK(item.getStateName() == "Suspended");
    item.resume();  CHECK(item.getStateName() == "InProgress");
    item.complete();
    item.reopen();  CHECK(item.getStateName() == "InProgress");
}

// ===================== STRATEGY =====================
TEST_CASE("Strategy: RoleBased picks someone holding the role") {
    WorkItem item("i1", "assess"); item.setRole("Assessor");
    Participant clerk = person("clerk", "Clerk"), ann = person("ann", "Assessor");
    std::vector<Participant*> pool; pool.push_back(&clerk); pool.push_back(&ann);
    RoleBasedStrategy s;
    CHECK(s.selectParticipant(item, pool) == &ann);
}

TEST_CASE("Strategy: nobody eligible gives nullptr") {
    WorkItem item("i1", "assess"); item.setRole("Assessor");
    Participant clerk = person("clerk", "Clerk");
    std::vector<Participant*> pool; pool.push_back(&clerk);
    std::vector<Participant*> empty;
    RoleBasedStrategy rb; ShortestQueueStrategy sq; RoundRobinStrategy rr;
    CHECK(rb.selectParticipant(item, pool) == nullptr);
    CHECK(sq.selectParticipant(item, pool) == nullptr);
    CHECK(rr.selectParticipant(item, pool) == nullptr);
    CHECK(rb.selectParticipant(item, empty) == nullptr);
}

TEST_CASE("Strategy: ShortestQueue picks the least busy person") {
    WorkItem item("i1", "assess"); item.setRole("Assessor");
    Participant busy = person("busy", "Assessor"), idle = person("idle", "Assessor");
    busy.itemAssigned(); busy.itemAssigned();
    std::vector<Participant*> pool; pool.push_back(&busy); pool.push_back(&idle);
    ShortestQueueStrategy s;
    CHECK(s.selectParticipant(item, pool) == &idle);
    idle.itemAssigned(); idle.itemAssigned(); idle.itemAssigned();
    CHECK(s.selectParticipant(item, pool) == &busy);
}

TEST_CASE("Strategy: workload goes up on assign and down when the item finishes") {
    WorkItem item("i1", "assess");
    Participant ann = person("ann", "Assessor");
    item.makeAvailable(); item.assign(&ann);
    CHECK(ann.getWorkload() == 1);
    item.start(); item.complete();
    CHECK(ann.getWorkload() == 0);
}

TEST_CASE("Strategy: RoundRobin rotates and wraps") {
    WorkItem item("i1", "assess"); item.setRole("Assessor");
    Participant a = person("a", "Assessor"), b = person("b", "Assessor");
    std::vector<Participant*> pool; pool.push_back(&a); pool.push_back(&b);
    RoundRobinStrategy s;
    CHECK(s.selectParticipant(item, pool) == &a);
    CHECK(s.selectParticipant(item, pool) == &b);
    CHECK(s.selectParticipant(item, pool) == &a);
}

TEST_CASE("Strategy: cloned RoundRobin strategies rotate independently") {
    WorkItem item("i1", "assess"); item.setRole("Assessor");
    Participant a = person("a", "Assessor"), b = person("b", "Assessor");
    std::vector<Participant*> pool; pool.push_back(&a); pool.push_back(&b);
    RoundRobinStrategy proto;
    AssignmentStrategy* c1 = proto.clone();
    AssignmentStrategy* c2 = proto.clone();
    c1->selectParticipant(item, pool);
    c1->selectParticipant(item, pool);
    CHECK(c2->selectParticipant(item, pool) == &a);   // c2 untouched by c1
    delete c1; delete c2;
}

TEST_CASE("Strategy: DataRule maps a data value to a role") {
    Participant motor = person("m", "MotorHandler"), home = person("h", "HomeHandler");
    std::vector<Participant*> pool; pool.push_back(&motor); pool.push_back(&home);
    DataRuleStrategy s("claimType");
    s.addMapping("motor", "MotorHandler");
    s.addMapping("home", "HomeHandler");
    WorkItem item("i1", "assess");
    item.setData("claimType", "home");
    CHECK(s.selectParticipant(item, pool) == &home);
    item.setData("claimType", "marine");               // not mapped
    CHECK(s.selectParticipant(item, pool) == nullptr);
}

TEST_CASE("WorkAssigner: item's rule overrides the default strategy") {
    Participant a = person("a", "Assessor"), b = person("b", "Assessor");
    std::vector<Participant*> pool; pool.push_back(&a); pool.push_back(&b);
    WorkAssigner assigner(nullptr);
    assigner.setDefaultStrategy(RoleBasedStrategy());
    assigner.registerStrategy("roundRobin", RoundRobinStrategy());
    WorkItem plain("p", "plain"); plain.setRole("Assessor");
    WorkItem rr("r", "rr"); rr.setRole("Assessor"); rr.setAssignmentRule("roundRobin");
    CHECK(assigner.assign(plain, pool) == &a);
    CHECK(assigner.assign(plain, pool) == &a);   // role-based: always the first
    CHECK(assigner.assign(rr, pool) == &a);
    CHECK(assigner.assign(rr, pool) == &b);      // round robin: rotates
}

TEST_CASE("WorkAssigner: no strategy at all gives nullptr") {
    WorkItem item("i1", "assess");
    std::vector<Participant*> pool;
    WorkAssigner none(nullptr);
    CHECK(none.assign(item, pool) == nullptr);
}

// ===================== MEDIATOR =====================
struct Flow {                                     // one fresh instance per test
    WorkAssigner assigner;
    InstanceCoordinator coord;
    Participant ann;
    Flow() : assigner(nullptr), coord(&assigner, nullptr), ann("ann", "ann") {
        ann.addRole("Assessor");
        assigner.setDefaultStrategy(RoleBasedStrategy());
        coord.addParticipant(&ann);
    }
};

TEST_CASE_FIXTURE(Flow, "Mediator: an item waits for its prerequisite") {
    WorkItem a("A", "assess"), b("B", "decide");
    a.setRole("Assessor"); b.setRole("Assessor");
    coord.registerItem(&a); coord.registerItem(&b);
    coord.addDependency("A", "B");                 // (prerequisite, item)
    CHECK_FALSE(coord.dependenciesMet("B"));
    coord.start();
    CHECK(a.getStateName() == "Assigned");
    CHECK(b.isPending());                          // B not released yet
    a.start(); a.complete();
    CHECK(coord.dependenciesMet("B"));
    CHECK(b.getStateName() == "Assigned");         // released and assigned
}

TEST_CASE_FIXTURE(Flow, "Mediator: an item needs ALL prerequisites") {
    WorkItem a("A", "x"), b("B", "y"), c("C", "z");
    a.setRole("Assessor"); b.setRole("Assessor"); c.setRole("Assessor");
    coord.registerItem(&a); coord.registerItem(&b); coord.registerItem(&c);
    coord.addDependency("A", "C"); coord.addDependency("B", "C");
    coord.start();
    a.start(); a.complete();
    CHECK(c.isPending());                          // B still outstanding
    b.start(); b.complete();
    CHECK(c.getStateName() == "Assigned");
}

TEST_CASE_FIXTURE(Flow, "Mediator: a rejected prerequisite escalates its dependents") {
    WorkItem a("A", "x"), b("B", "y"), c("C", "z");
    a.setRole("Assessor"); b.setRole("Assessor"); c.setRole("Assessor");
    coord.registerItem(&a); coord.registerItem(&b); coord.registerItem(&c);
    coord.addDependency("A", "B"); coord.addDependency("B", "C");
    coord.start();
    a.start(); a.reject("invalid claim");
    CHECK(b.getStateName() == "Escalated");
    CHECK(c.getStateName() == "Escalated");        // follows the chain
    CHECK(coord.isFinished());
}

TEST_CASE_FIXTURE(Flow, "Mediator: nobody eligible takes the exception path") {
    WorkItem a("A", "x"); a.setRole("Pilot");      // no participant has this role
    coord.registerItem(&a);
    CHECK_NOTHROW(coord.start());
    CHECK(a.getStateName() == "Escalated");
}

TEST_CASE_FIXTURE(Flow, "Mediator: duplicate and null items are refused") {
    WorkItem a("A", "x");
    CHECK(coord.registerItem(&a));
    CHECK_FALSE(coord.registerItem(&a));
    CHECK_FALSE(coord.registerItem(nullptr));
}

TEST_CASE("Mediator: two instances do not share state") {
    Flow f1, f2;
    WorkItem a1("A", "x"), b1("B", "y"), a2("A", "x"), b2("B", "y");
    a1.setRole("Assessor"); b1.setRole("Assessor");
    a2.setRole("Assessor"); b2.setRole("Assessor");
    f1.coord.registerItem(&a1); f1.coord.registerItem(&b1); f1.coord.addDependency("A", "B");
    f2.coord.registerItem(&a2); f2.coord.registerItem(&b2); f2.coord.addDependency("A", "B");
    f1.coord.start(); f2.coord.start();
    a1.start(); a1.complete();
    CHECK(b1.getStateName() == "Assigned");
    CHECK(b2.isPending());                         // instance 2 unaffected
    CHECK_FALSE(f2.coord.dependenciesMet("B"));
}