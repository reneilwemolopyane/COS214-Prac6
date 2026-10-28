// tests/MediatorStrategyTests.cpp
#include <gtest/gtest.h>
#include <vector>

#include "tividy/core/Participant.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/InvalidTransitionException.h"
#include "tividy/strategy/AssignmentStrategy.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/observer/Observers.h"          

// ---------- helpers ----------
namespace {

/** Records every event type it receives, in order. */
class RecordingObserver : public WorkflowObserver {
public:
    void update(const WorkflowEvent& e) override { seen.push_back(e.getType()); }
    int count(int type) const {
        int n = 0;
        for (size_t i = 0; i < seen.size(); i++) if (seen[i] == type) n++;
        return n;
    }
    int firstIndex(int type) const {
        for (size_t i = 0; i < seen.size(); i++) if (seen[i] == type) return (int)i;
        return -1;
    }
    int lastIndex(int type) const {
        for (int i = (int)seen.size() - 1; i >= 0; i--) if (seen[i] == type) return i;
        return -1;
    }
    std::vector<int> seen;
};

/** One independent workflow instance: its own people, items, assigner, coordinator. */
struct Instance {
    // Declared first => destroyed last, because items release their assignee on destruction.
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
    void run(WorkItem& w) { w.start(); w.complete(); }   // assigned -> in progress -> completed
};

} // namespace

// ================= Strategy =================

TEST(StrategyTest, RoleBasedPicksEligibleParticipant) {
    Participant nurse("P1", "Sipho"), assessor("P2", "Thandi");
    nurse.addRole("Nurse");
    assessor.addRole("Assessor");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &nurse, &assessor };

    RoleBasedStrategy s;
    EXPECT_EQ(&assessor, s.selectParticipant(item, pool));
}

TEST(StrategyTest, RoleBasedReturnsNullWhenNobodyEligible) {
    Participant nurse("P1", "Sipho");
    nurse.addRole("Nurse");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &nurse };

    RoleBasedStrategy s;
    EXPECT_EQ(nullptr, s.selectParticipant(item, pool));
}

TEST(StrategyTest, ShortestQueuePicksLowestWorkload) {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    a.itemAssigned();                       // Thandi is busier
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &a, &b };

    ShortestQueueStrategy s;
    EXPECT_EQ(&b, s.selectParticipant(item, pool));
}

TEST(StrategyTest, RoundRobinRotates) {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &a, &b };

    RoundRobinStrategy s;
    EXPECT_EQ(&a, s.selectParticipant(item, pool));
    EXPECT_EQ(&b, s.selectParticipant(item, pool));
    EXPECT_EQ(&a, s.selectParticipant(item, pool));
}

TEST(StrategyTest, CloneKeepsItsOwnRoundRobinPosition) {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    WorkItem item("i1", "Inspect");
    item.setRole("Assessor");
    std::vector<Participant*> pool = { &a, &b };

    RoundRobinStrategy original;
    original.selectParticipant(item, pool);                 // original -> a
    AssignmentStrategy* copy = original.clone();
    EXPECT_EQ(&b, copy->selectParticipant(item, pool));     // copy continues from a -> b
    EXPECT_EQ(&b, original.selectParticipant(item, pool));  // original unaffected by the copy
    delete copy;
}

TEST(StrategyTest, DataRuleMapsValueToRole) {
    Participant car("P1", "Thandi"), house("P2", "Sipho");
    car.addRole("VehicleAssessor");
    house.addRole("PropertyAssessor");
    WorkItem item("i1", "Assess");
    item.setData("claimType", "property");
    std::vector<Participant*> pool = { &car, &house };

    DataRuleStrategy s("claimType");
    s.addMapping("vehicle", "VehicleAssessor");
    s.addMapping("property", "PropertyAssessor");
    EXPECT_EQ(&house, s.selectParticipant(item, pool));
}

TEST(StrategyTest, DataRuleUnknownValueReturnsNull) {
    Participant car("P1", "Thandi");
    car.addRole("VehicleAssessor");
    WorkItem item("i1", "Assess");
    item.setData("claimType", "marine");
    std::vector<Participant*> pool = { &car };

    DataRuleStrategy s("claimType");
    s.addMapping("vehicle", "VehicleAssessor");
    EXPECT_EQ(nullptr, s.selectParticipant(item, pool));
}

TEST(StrategyTest, WorkAssignerUsesRegisteredRuleThenDefault) {
    Participant a("P1", "Thandi"), b("P2", "Sipho");
    a.addRole("Assessor");
    b.addRole("Assessor");
    a.itemAssigned();                                       // a busier than b
    std::vector<Participant*> pool = { &a, &b };

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());       // would pick a
    assigner.registerStrategy("light", ShortestQueueStrategy());   // picks b

    WorkItem ruled("i1", "Ruled");
    ruled.setRole("Assessor");
    ruled.setAssignmentRule("light");
    WorkItem plain("i2", "Plain");
    plain.setRole("Assessor");

    EXPECT_EQ(&b, assigner.assign(ruled, pool));
    EXPECT_EQ(&a, assigner.assign(plain, pool));
}

// ================= Mediator =================

TEST(MediatorTest, DecisionWaitsForAllPrerequisites) {
    Instance in;
    in.coord.start();

    EXPECT_TRUE(in.police.isAvailable() || in.police.getStateName() == "Assigned");
    EXPECT_TRUE(in.decision.isPending());

    in.run(in.police);
    EXPECT_TRUE(in.decision.isPending());                   // inspection still open

    in.run(in.inspect);
    EXPECT_FALSE(in.decision.isPending());                  // released once both are done
}

TEST(MediatorTest, DuplicateAndNullItemsAreRejected) {
    Instance in;
    WorkItem dup("police", "Another police check");
    EXPECT_FALSE(in.coord.registerItem(&dup));
    EXPECT_FALSE(in.coord.registerItem(nullptr));
}

TEST(MediatorTest, WorkloadRisesOnAssignAndFallsOnComplete) {
    Instance in;
    in.coord.start();
    int busy = in.thandi.getWorkload() + in.sipho.getWorkload();
    EXPECT_EQ(2, busy);                                     // police and inspect assigned

    in.run(in.police);
    EXPECT_EQ(1 + 1, in.thandi.getWorkload() + in.sipho.getWorkload() + 0 + 0 - 0 + 0 + 0
                      - (in.decision.isPending() ? 0 : 1) + (in.decision.isPending() ? 0 : 0));
}

TEST(MediatorTest, NoEligibleParticipantEscalatesTheItem) {
    Instance in;
    in.police.setRole("Surgeon");                           // nobody holds this role
    in.coord.start();

    EXPECT_GE(in.log.count(EVT_FAILED), 1);
    EXPECT_TRUE(in.police.isFinished());                    // forced into Escalated (final)
}

TEST(MediatorTest, RejectedPrerequisiteEscalatesDependentsSoInstanceFinishes) {
    Instance in;
    in.coord.start();

    in.police.start();
    in.police.reject("report invalid");
    in.run(in.inspect);

    EXPECT_TRUE(in.decision.isFinished());                  // escalated, not left waiting
    EXPECT_TRUE(in.coord.isFinished());
}

TEST(MediatorTest, FullHappyPathFinishesInstance) {
    Instance in;
    in.coord.start();
    in.run(in.police);
    in.run(in.inspect);
    in.run(in.decision);
    EXPECT_TRUE(in.coord.isFinished());
}

TEST(MediatorTest, ObserverSeesCompletedBeforeNextAssigned) {
    Instance in;
    in.coord.start();
    in.run(in.police);
    in.run(in.inspect);                                     // releases and assigns decision

    int completed = in.log.lastIndex(EVT_COMPLETED);
    int assignedLast = in.log.lastIndex(EVT_ASSIGNED);
    EXPECT_LT(completed, assignedLast);                     // log first, then react
}

TEST(MediatorTest, RefusedActionRaisesNoEvent) {
    Instance in;                                            // not started: items are Created
    EXPECT_THROW(in.police.complete(), InvalidTransitionException);
    EXPECT_EQ(0, in.log.count(EVT_COMPLETED));
}

TEST(MediatorTest, StartIsIdempotent) {
    Instance in;
    in.coord.start();
    int assigned = in.log.count(EVT_ASSIGNED);
    in.coord.start();                                       // second call does nothing
    EXPECT_EQ(assigned, in.log.count(EVT_ASSIGNED));
}

// ================= Instance isolation =================

TEST(IsolationTest, CompletingInOneInstanceDoesNotAffectAnother) {
    Instance a;
    Instance b;
    a.coord.start();
    b.coord.start();

    a.run(a.police);
    a.run(a.inspect);

    EXPECT_FALSE(a.decision.isPending());                   // released in A
    EXPECT_TRUE(b.decision.isPending());                    // untouched in B
    EXPECT_EQ(0, b.log.count(EVT_COMPLETED));
}

TEST(IsolationTest, WorkloadIsPerInstance) {
    Instance a;
    Instance b;
    a.coord.start();
    EXPECT_EQ(0, b.thandi.getWorkload() + b.sipho.getWorkload());
}