// Copy the exact doctest include line from tests/chain_decorator/test_decorators.cpp.
// If your test target has no main(), put this above it:
// #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/decorator/Decorators.h"
#include "tividy/chain_of_responsibility/Approval.h"
#include "tividy/visitor/BottleneckVisitor.h"

namespace {

int countOf(const std::string& text, const std::string& needle) {
    int n = 0;
    std::string::size_type pos = 0;
    while ((pos = text.find(needle, pos)) != std::string::npos) {
        ++n;
        pos += needle.size();
    }
    return n;
}

// Minimal visitor so these tests don't need the memento/instance sources.
class CollectingVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage& s) override { out += "Stage: " + s.getName() + "\n"; }
    void visitWorkItem(WorkItem& i) override {
        out += "Work item " + i.getId() + ": " + i.getName() + "\n";
    }
    std::string getReport() const override { return out; }
private:
    std::string out;
};

// Stage activates the item (Available), then Assigned, then Started via the top decorator.
void startItem(Stage& stage, WorkItem* item, WorkflowComponent* top) {
    stage.start();
    item->assign();
    top->start();
}

} // namespace

TEST_CASE("Visitor reaches a decorated item through the composite") {
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Inspect damage");
    stage.add(new PriorityDecorator(new AuditDecorator(item), 3));

    CollectingVisitor v;
    stage.accept(v);
    std::string report = v.getReport();
    CHECK(report.find("Stage: Claim") != std::string::npos);
    CHECK(report.find("Work item w1: Inspect damage") != std::string::npos);
}

TEST_CASE("BottleneckVisitor reports an overdue decorated item") {
    long now = 100;
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Inspect damage");
    WorkflowComponent* top = new DeadlineDecorator(item, 10, [&now]() { return now; });
    stage.add(top);
    startItem(stage, item, top);

    BottleneckVisitor early;
    stage.accept(early);
    CHECK(early.getReport().find("[BOTTLENECK] Work item Inspect damage") == std::string::npos);

    now = 200;   // 100 s after start, limit is 10 s
    BottleneckVisitor late;
    stage.accept(late);
    CHECK(late.getReport().find("[BOTTLENECK] Work item Inspect damage") != std::string::npos);
}

TEST_CASE("Stacked decorators report an overdue item only once") {
    long now = 0;
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Inspect damage");
    WorkflowComponent* top = new PriorityDecorator(
        new NotificationDecorator(
            new DeadlineDecorator(item, 10, [&now]() { return now; }),
            [](const std::string&) {}),
        2);
    stage.add(top);
    startItem(stage, item, top);
    now = 50;

    BottleneckVisitor v;
    stage.accept(v);
    CHECK(countOf(v.getReport(), "[BOTTLENECK] Work item Inspect damage") == 1);
}

TEST_CASE("Approval chain approves at the right level and the item completes") {
    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000);
    handler.setSuccessor(&manager);

    double amount = 50000;
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Authorise repair");
    ApprovalDecorator* approval = new ApprovalDecorator(
        item, &handler, [&amount]() { return amount; });
    stage.add(approval);
    startItem(stage, item, approval);

    approval->complete("authorised");

    CHECK(approval->lastOutcome() == APPROVED);
    CHECK(approval->approvedBy() == "Manager");
    CHECK(item->isComplete());
    CHECK(stage.isComplete());
}

TEST_CASE("Approver rejection rejects the item and the stage reports failure") {
    LevelApprover handler("Handler", 20000,
        [](ApprovalRequest& r) -> ApprovalOutcome {
            r.reason = "Fraud suspected";
            return REJECTED;
        });

    double amount = 1000;
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Authorise repair");
    ApprovalDecorator* approval = new ApprovalDecorator(
        item, &handler, [&amount]() { return amount; });
    stage.add(approval);
    startItem(stage, item, approval);

    approval->complete("authorised");

    CHECK(approval->lastOutcome() == REJECTED);
    CHECK_FALSE(item->isComplete());
    CHECK(item->isFailed());
    CHECK(stage.isFailed());
    CHECK_FALSE(stage.isComplete());
}

TEST_CASE("Amount above every limit applies the default decision") {
    LevelApprover handler("Handler", 20000);

    double amount = 500000;
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Authorise repair");
    ApprovalDecorator* approval = new ApprovalDecorator(
        item, &handler, [&amount]() { return amount; }, REJECTED);
    stage.add(approval);
    startItem(stage, item, approval);

    approval->complete("authorised");

    CHECK(approval->lastOutcome() == REJECTED);
    CHECK(item->isFailed());
}

TEST_CASE("Overdue item skips the first approval level and escalates") {
    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000);
    handler.setSuccessor(&manager);

    long now = 100;
    double amount = 1000;   // handler could normally approve this
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Authorise repair");
    WorkflowComponent* deadline = new DeadlineDecorator(item, 10, [&now]() { return now; });
    ApprovalDecorator* approval = new ApprovalDecorator(
        deadline, &handler, [&amount]() { return amount; });
    stage.add(approval);
    startItem(stage, item, approval);

    now = 500;   // deadline passed
    approval->complete("authorised");

    CHECK(approval->lastOutcome() == APPROVED);
    CHECK(approval->approvedBy() == "Manager");
    CHECK(item->isComplete());
}

TEST_CASE("Denied action is recorded by the audit decorator and the item is unchanged") {
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Inspect damage");
    AuditDecorator* audit = new AuditDecorator(
        new SecurityDecorator(item, "assessor", "clerk"));
    stage.add(audit);
    stage.start();

    CHECK_THROWS_AS(audit->complete("done"), std::runtime_error);

    REQUIRE(audit->history().size() == 2);
    CHECK(audit->history()[1].find("FAILED") != std::string::npos);
    CHECK_FALSE(item->isComplete());
}

TEST_CASE("Cloning a decorated stage gives an independent copy") {
    Stage stage("Claim");
    WorkItem* item = new WorkItem("w1", "Inspect damage");
    stage.add(new NotificationDecorator(item, [](const std::string&) {}));

    const std::string before = item->getStateName();

    WorkflowComponent* copy = stage.clone();
    copy->start();   // activates only the copy's items

    CHECK(item->getStateName() == before);   // original untouched
    CHECK_FALSE(item->isAvailable());
    CHECK_FALSE(stage.isFinished());
    delete copy;
}

TEST_CASE("WorkItem role and assignment rule survive cloning") {
    WorkItem item("w1", "Inspect damage");
    item.setRole("Assessor");
    item.setAssignmentRule("roundRobin");

    std::unique_ptr<WorkflowComponent> copy(item.clone());
    WorkItem* c = dynamic_cast<WorkItem*>(copy.get());

    REQUIRE(c != 0);
    CHECK(c->getRole() == "Assessor");
    CHECK(c->getAssignmentRule() == "roundRobin");
}