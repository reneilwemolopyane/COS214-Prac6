#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "composite_prototype/Stage.h"
#include "composite_prototype/WorkItem.h"
#include "state/InvalidTransitionException.h"
#include "core/Participant.h"

// Wrapped in an anonymous namespace so it doesn't clash with other test files
namespace {
    class StubLeaf : public WorkflowComponent {
    public:
        bool activated = false, finished = false, isDone = false;
        
        StubLeaf() {}
        
        void start() override { activate(); }
        void complete() override { 
            finished = true; isDone = true; 
            if (getParent()) getParent()->childFinished(*this); 
        }
        void cancel() override { 
            finished = true; isDone = false; 
            if (getParent()) getParent()->childFinished(*this); 
        }
        
        bool isComplete() const override { return isDone; }
        bool isFinished() const override { return finished; }
        bool isFailed() const override { return false; }
        std::string getName() const override { return "Stub"; }
        WorkflowComponent* clone() const override { return new StubLeaf(); }
        void accept(WorkflowVisitor&) override {}
        void activate() override { activated = true; }
    };
}


// COMPOSITE TESTS

TEST_CASE("Composite: Sequential Push Model") {
    Stage s("Seq", SEQUENTIAL);
    StubLeaf* a = new StubLeaf();
    StubLeaf* b = new StubLeaf();
    s.add(a); s.add(b);
    
    s.start();
    CHECK(a->activated == true);
    CHECK(b->activated == false);
    
    a->complete(); 
    CHECK(b->activated == true);
    CHECK(s.isFinished() == false);
    
    b->complete(); 
    CHECK(s.isFinished() == true);
}

TEST_CASE("Composite: Parallel Push Model") {
    Stage s("Par", PARALLEL);
    StubLeaf* a = new StubLeaf();
    StubLeaf* b = new StubLeaf();
    s.add(a); s.add(b);
    
    s.start();
    CHECK(a->activated == true);
    CHECK(b->activated == true);
    
    a->complete(); 
    CHECK(s.isFinished() == false);
    
    b->complete(); 
    CHECK(s.isFinished() == true);
}


// STATE TESTS

TEST_CASE("State: Happy Path & Workload Tracking") {
    WorkItem item("id_1", "Task A");
    Participant p("user1", "John"); 
    
    CHECK(item.getStateName() == "Created");
    CHECK(p.getWorkload() == 0);
    
    item.makeAvailable();
    item.assign(&p);
    CHECK(item.getStateName() == "Assigned");
    CHECK(p.getWorkload() == 1); 
    
    item.start();
    item.complete();
    
    CHECK(item.getStateName() == "Completed");
    CHECK(item.isFinished() == true);
    CHECK(p.getWorkload() == 0); 
}

TEST_CASE("State: Invalid Transitions Throw") {
    WorkItem item("id_1", "Task A");
    CHECK_THROWS_AS(item.start(), InvalidTransitionException);
    CHECK(item.getStateName() == "Created");
}

TEST_CASE("State: Escalation and Suspension") {
    WorkItem item("id_2", "Task B");
    item.makeAvailable();
    item.assign();
    item.escalate("Timeout");
    CHECK(item.getStateName() == "Escalated");

    WorkItem item2("id_3", "Task C");
    item2.makeAvailable();
    item2.assign();
    item2.start();
    item2.suspend();
    CHECK(item2.getStateName() == "Suspended");
    item2.resume();
    CHECK(item2.getStateName() == "InProgress");
}

TEST_CASE("State: Data Map (unblocking other patterns)") {
    WorkItem item("id_4", "Task D");
    CHECK(item.getData("amount") == "");
    item.setData("amount", "25000");
    CHECK(item.getData("amount") == "25000");
}

TEST_CASE("State: Clone produces fresh state") {
    WorkItem item("id_5", "Task E");
    item.makeAvailable();
    item.assign();
    item.start();
    item.complete();
    
    WorkflowComponent* rawClone = item.clone();
    WorkItem* clone = dynamic_cast<WorkItem*>(rawClone);
    
    REQUIRE(clone != nullptr);
    CHECK(clone->getStateName() == "Created"); 
    CHECK(clone->getAssignee() == nullptr);
    
    delete rawClone;
}