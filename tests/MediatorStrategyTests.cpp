#include "doctest.h"
#include <vector>

#include "core/Participant.h"
#include "core/Common.h"
#include "strategy/AssignmentStrategy.h"
#include "strategy/WorkAssigner.h"
#include "mediator/InstanceCoordinator.h"
#include "observer/Observers.h"
#include "composite_prototype/WorkItem.h"
#include "state/InvalidTransitionException.h"

namespace {

class RecordingObserver : public WorkflowObserver {
public:
    void update(const WorkflowEvent& e) override { seen.push_back(static_cast<int>(e.getType())); }
    int count(int type) const {
        int n = 0;
        for (size_t i = 0; i < seen.size(); i++) if (seen[i] == type) n++;
        return n;
    }
    int lastIndex(int type) const {
        for (int i = (int)seen.size() - 1; i >= 0; i--) if (seen[i] == type) return i;
        return -1;
    }
    std::vector<int> seen;
};

// One independent workflow instance. Participants are declared first so they
// are destroyed last (items release their assignee when destroyed).
struct Instance {
    Participant thandi;
    Participant sipho;
    WorkItem police;
    WorkItem inspect;
    WorkItem decision;
    WorkAssigner assigner;
    WorkflowSubject subject;
    RecordingObserver log;
    InstanceCoordinator coord;

    Instance()
        : thandi("P1", "Thandi"), sipho("P2", "Sipho"),
          police("police", "Police check"),
          inspect("inspect", "Inspection"),
          decision("decision", "Decision"),
          coord(&assigner, &subject) {
        thandi.addRole("Assessor");
        sipho.addRole("Assessor");
        police.setRole("Assessor");
        inspect.setRole("Assessor");
        decision.setRole("Assessor");
        assigner.setDefaultStrategy(RoleBasedStrategy());
        subject.attach(EVT_COMPLETED, &log);
        subject.attach(EVT_ASSIGNED, &log);
        subject.attach(EVT_FAILED, &log);
        subject.attach(EVT_ESCALATED, &log);
        subject.attach(EVT_REJECTED, &log);
        coord.addParticipant(&thandi);
        coord.addParticipant(&sipho);
        coord.registerItem(&police);
        coord.registerItem(&inspect);
        coord.registerItem(&decision);
        coord.addDependency("police", "decision");
        coord.addDependency("inspect", "decision");
    }
    void run(WorkItem& w) { w.start(); w.complete(); }
};

} // namespace

// ================= Strategy =================

TEST_CASE("Strategy: RoleBased picks eligible participant") {
    Participant nurse("P1", "Sipho"), assessor("P2", "Thandi");
    nurse.addRole("Nurse");
    assessor.addRole("Assessor");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &nurse, &assessor };

    RoleBasedStrategy s;
    CHECK(s.selectParticipant(item, pool) == &assessor);
}

TEST_CASE("Strategy: RoleBased returns null when nobody eligible") {
    Participant nurse("P1", "Sipho");
    nurse.addRole("Nurse");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &nurse };

    RoleBasedStrategy s;
    CHECK(s.selectParticipant(item, pool) == nullptr);
}

TEST_CASE("Strategy: ShortestQueue picks lowest workload") {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    a.itemAssigned();
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &a, &b };

    ShortestQueueStrategy s;
    CHECK(s.selectParticipant(item, pool) == &b);
}

TEST_CASE("Strategy: RoundRobin rotates") {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &a, &b };

    RoundRobinStrategy s;
    CHECK(s.selectParticipant(item, pool) == &a);
    CHECK(s.selectParticipant(item, pool) == &b);
    CHECK(s.selectParticipant(item, pool) == &a);
}

TEST_CASE("Strategy: clone keeps its own RoundRobin position") {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &a, &b };

    RoundRobinStrategy original;
    original.selectParticipant(item, pool);
    AssignmentStrategy* copy = original.clone();
    CHECK(copy->selectParticipant(item, pool) == &b);
    CHECK(original.selectParticipant(item, pool) == &b);
    delete copy;
}

TEST_CASE("Strategy: DataRule maps value to role") {
    Participant car("P1", "Thandi"), house("P2", "Sipho");
    car.addRole("VehicleAssessor");
    house.addRole("PropertyAssessor");
    WorkItem item("i1", "Assess");
    item.setData("claimType", "property");
    std::vector<Participant*> pool = { &car, &house };

    DataRuleStrategy s("claimType");
    s.addMapping("vehicle", "VehicleAssessor");
    s.addMapping("property", "PropertyAssessor");
    CHECK(s.selectParticipant(item, pool) == &house);
}

TEST_CASE("Strategy: DataRule unknown value returns null") {
    Participant car("P1", "Thandi");
    car.addRole("VehicleAssessor");
    WorkItem item("i1", "Assess");
    item.setData("claimType", "marine");
    std::vector<Participant*> pool = { &car };

    DataRuleStrategy s("claimType");
    s.addMapping("vehicle", "VehicleAssessor");
    CHECK(s.selectParticipant(item, pool) == nullptr);
}

TEST_CASE("Strategy: WorkAssigner uses registered rule, then default") {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    a.itemAssigned();
    std::vector<Participant*> pool = { &a, &b };

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());
    assigner.registerStrategy("light", ShortestQueueStrategy());

    WorkItem ruled("i1", "Ruled");
    ruled.setRole("Assessor");
    ruled.setAssignmentRule("light");
    WorkItem plain("i2", "Plain");
    plain.setRole("Assessor");

    CHECK(assigner.assign(ruled, pool) == &b);
    CHECK(assigner.assign(plain, pool) == &a);
}

// ================= Mediator =================

TEST_CASE("Mediator: decision waits for all prerequisites") {
    Instance in;
    in.coord.start();
    CHECK(in.decision.isPending());

    in.run(in.police);
    CHECK(in.decision.isPending());

    in.run(in.inspect);
    CHECK_FALSE(in.decision.isPending());
}

TEST_CASE("Mediator: duplicate and null items are rejected") {
    Instance in;
    WorkItem dup("police", "Another police check");
    CHECK_FALSE(in.coord.registerItem(&dup));
    CHECK_FALSE(in.coord.registerItem(nullptr));
}

TEST_CASE("Mediator: workload rises on assign and falls on complete") {
    Instance in;
    in.coord.start();
    CHECK_EQ(2, in.thandi.getWorkload() + in.sipho.getWorkload());

    in.run(in.police);
    CHECK_EQ(1, in.thandi.getWorkload() + in.sipho.getWorkload());
}

TEST_CASE("Mediator: no eligible participant escalates the item") {
    Instance in;
    in.police.setRole("Surgeon");
    in.coord.start();

    CHECK_GE(in.log.count(EVT_FAILED), 1);
    CHECK(in.police.isFinished());
}

TEST_CASE("Mediator: rejected prerequisite escalates dependents") {
    Instance in;
    in.coord.start();
    in.police.start();
    in.police.reject("report invalid");
    in.run(in.inspect);

    CHECK(in.decision.isFinished());
    CHECK(in.coord.isFinished());
}

TEST_CASE("Mediator: full happy path finishes the instance") {
    Instance in;
    in.coord.start();
    in.run(in.police);
    in.run(in.inspect);
    in.run(in.decision);
    CHECK(in.coord.isFinished());
}

TEST_CASE("Mediator: observer sees COMPLETED before next ASSIGNED") {
    Instance in;
    in.coord.start();
    in.run(in.police);
    in.run(in.inspect);

    CHECK_LT(in.log.lastIndex(EVT_COMPLETED), in.log.lastIndex(EVT_ASSIGNED));
}

TEST_CASE("Mediator: refused action raises no event") {
    Instance in;
    CHECK_THROWS_AS(in.police.complete(), InvalidTransitionException);
    CHECK_EQ(0, in.log.count(EVT_COMPLETED));
}

TEST_CASE("Mediator: start is idempotent") {
    Instance in;
    in.coord.start();
    int assigned = in.log.count(EVT_ASSIGNED);
    in.coord.start();
    CHECK_EQ(assigned, in.log.count(EVT_ASSIGNED));
}

// ================= Instance isolation =================

TEST_CASE("Isolation: completing in one instance does not affect another") {
    Instance a;
    Instance b;
    a.coord.start();
    b.coord.start();

    a.run(a.police);
    a.run(a.inspect);

    CHECK_FALSE(a.decision.isPending());
    CHECK(b.decision.isPending());
    CHECK_EQ(0, b.log.count(EVT_COMPLETED));
}

TEST_CASE("Isolation: workload is per instance") {
    Instance a;
    Instance b;
    a.coord.start();
    CHECK_EQ(0, b.thandi.getWorkload() + b.sipho.getWorkload());
}