#include "doctest.h"

#include <string>
#include <vector>

#include "tividy/builder/WorkflowDirector.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/facade/WorkflowFacade.h"
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/command/WorkflowEngine.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/core/Participant.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/strategy/AssignmentStrategy.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/observer/Observers.h"
#include "tividy/adapter/PaymentAPI.h"
#include "tividy/adapter/PaymentAdapter.h"
#include "tividy/decorator/Decorators.h"
#include "tividy/chain_of_responsibility/Approval.h"
#include "tividy/visitor/BottleneckVisitor.h"
#include "tividy/memento/Memento.h"
#include "tividy/memento/ProcessDataStore.h"

namespace {

class ItemFinder : public WorkflowVisitor {
public:
    explicit ItemFinder(const std::string& workItemId) : id(workItemId), found(0) {}
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    void visitWorkItem(WorkItem& i) override { if (i.getId() == id) found = &i; }
    void visitDecorator(WorkflowComponent& c) override { layers.push_back(&c); }
    std::string getReport() const override { return ""; }

    std::string id;
    WorkItem* found;
    std::vector<WorkflowComponent*> layers;
};

WorkItem* findItem(WorkflowInstance* instance, const std::string& id) {
    ItemFinder f(id);
    instance->accept(f);
    return f.found;
}

class ClaimDirector : public WorkflowDirector {
public:
    void construct(WorkflowBuilder& builder) override {
        builder.setIdentity("all-patterns-claim", 1);
        builder.buildStage("Claim");
        builder.buildWorkItem("Lodge claim", "Policyholder");
        builder.buildWorkItem("Authorise payout", "FinanceClerk");
        builder.buildDependency("Lodge claim", "Authorise payout");
        builder.buildRule("payoutLimit", "50000");
    }
};

}

TEST_CASE("All patterns: Director, Builder and Facade build a validated claim; Prototype keeps two runs independent") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    ValidatingWorkflowBuilder builder;
    ClaimDirector director;
    director.construct(builder);
    WorkflowDefinition* def = builder.getResult();
    REQUIRE(def->isValidated());
    REQUIRE(def->getDependencies().size() == 1);
    facade.registerDefinition(def);

    std::string idA = facade.startWorkflow("all-patterns-claim", DataMap());
    std::string idB = facade.startWorkflow("all-patterns-claim", DataMap());
    CHECK(idA == "all-patterns-claim-1");
    CHECK(idB == "all-patterns-claim-2");

    WorkflowInstance* a = engine.findInstance(idA);
    WorkflowInstance* b = engine.findInstance(idB);
    REQUIRE(a != 0);
    REQUIRE(b != 0);
    REQUIRE(a != b);

    WorkItem* lodgeA = findItem(a, "Lodge claim");
    WorkItem* payoutA = findItem(a, "Authorise payout");
    REQUIRE(lodgeA != 0);
    REQUIRE(payoutA != 0);
    CHECK(lodgeA->getStateName() == "Available");

    a->getCoordinator()->addDependency("Lodge claim", "Authorise payout");
    CHECK_FALSE(a->getCoordinator()->dependenciesMet("Authorise payout"));

    Participant mo("P1", "Mo");
    engine.assignWorkItem(a, lodgeA, &mo);
    lodgeA->start();
    engine.completeWorkItem(a, lodgeA, "lodged");

    CHECK(a->getCoordinator()->dependenciesMet("Authorise payout"));

    WorkItem* lodgeB = findItem(b, "Lodge claim");
    REQUIRE(lodgeB != 0);
    CHECK(lodgeB != lodgeA);
    CHECK(lodgeB->getStateName() == "Available");
}

TEST_CASE("All patterns: Strategy picks the assignee, Command performs it, Observer records it, and the item moves through every state") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    ValidatingWorkflowBuilder builder;
    builder.setIdentity("assignment-claim", 1);
    builder.buildStage("Assessment");
    builder.buildWorkItem("Assess damage", "Assessor");
    facade.registerDefinition(builder.getResult());

    std::string id = facade.startWorkflow("assignment-claim", DataMap());
    WorkflowInstance* instance = engine.findInstance(id);
    REQUIRE(instance != 0);
    AuditLogger audit(instance);

    WorkItem* item = findItem(instance, "Assess damage");
    REQUIRE(item != 0);
    item->setAssignmentRule("shortestQueue");

    Participant busy("P1", "Busy"); busy.addRole("Assessor");
    Participant idle("P2", "Idle"); idle.addRole("Assessor");
    busy.itemAssigned(); busy.itemAssigned();
    std::vector<Participant*> pool;
    pool.push_back(&busy);
    pool.push_back(&idle);

    WorkAssigner assigner(0);
    assigner.setDefaultStrategy(RoleBasedStrategy());
    assigner.registerStrategy("shortestQueue", ShortestQueueStrategy());

    Participant* chosen = assigner.assign(*item, pool);
    REQUIRE(chosen == &idle);

    CHECK(engine.assignWorkItem(instance, item, chosen));
    CHECK(item->getStateName() == "Assigned");
    CHECK(idle.getWorkload() == 1);

    item->start();
    CHECK(engine.completeWorkItem(instance, item, "no damage found"));
    CHECK(item->getStateName() == "Completed");
    CHECK(idle.getWorkload() == 0);

    bool sawAssigned = false, sawCompleted = false;
    const std::vector<WorkflowEvent>& trail = audit.getTrail();
    for (std::size_t i = 0; i < trail.size(); ++i) {
        if (trail[i].getType() == EVT_ASSIGNED && trail[i].get("command") == "AssignWorkItem") sawAssigned = true;
        if (trail[i].getType() == EVT_COMPLETED && trail[i].get("command") == "CompleteWorkItem") sawCompleted = true;
    }
    CHECK(sawAssigned);
    CHECK(sawCompleted);
}

TEST_CASE("All patterns: an Adapter outage is retried, Command compensates it on cancel, and Memento keeps the data from before it happened") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    ValidatingWorkflowBuilder builder;
    builder.setIdentity("payout-claim", 1);
    builder.buildStage("Payout");
    builder.buildWorkItem("Authorise payout", "FinanceClerk");
    facade.registerDefinition(builder.getResult());

    DataMap data;
    data["claimant"] = "Mo";
    std::string id = facade.startWorkflow("payout-claim", data);
    WorkflowInstance* instance = engine.findInstance(id);
    REQUIRE(instance != 0);
    AuditLogger audit(instance);
    IncidentMonitor monitor(instance, instance->getProcessor(), 3);

    PaymentAPI bank;
    PaymentAdapter gateway("Bank", &bank);
    ConnectorRequest pay;
    pay.operation = "pay";
    pay.params["amount"] = "20000";
    pay.params["account"] = "ACC-9";

    bank.setDown(true);
    CHECK_FALSE(engine.invokeService(instance, &gateway, pay, "Authorise payout", "reverse"));
    CHECK(monitor.getIncidents().size() == 1);

    bool sawFailed = false;
    for (std::size_t i = 0; i < audit.getTrail().size(); ++i)
        if (audit.getTrail()[i].getType() == EVT_FAILED) sawFailed = true;
    CHECK(sawFailed);

    InstanceMemento* beforePayment = instance->createMemento();

    bank.setDown(false);
    CHECK(engine.invokeService(instance, &gateway, pay, "Authorise payout", "reverse"));
    REQUIRE(bank.knows("TXN-1"));

    CHECK(engine.cancel(id));
    CHECK(engine.getStatus(id) == "CANCELLED");

    bool sawCompensated = false;
    for (std::size_t i = 0; i < audit.getTrail().size(); ++i)
        if (audit.getTrail()[i].getType() == EVT_COMPENSATED) sawCompensated = true;
    CHECK(sawCompensated);
    CHECK_FALSE(bank.reverse("TXN-1"));

    instance->restoreFromMemento(*beforePayment);
    InstanceMemento* now = instance->createMemento();
    CHECK(now->getData().at("claimant") == "Mo");
    delete now;
    delete beforePayment;
}

TEST_CASE("All patterns: the decorator stack and approval chain gate completion inside a real, started instance; Visitor reports it and the Memento store keeps its history") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    LevelApprover clerk("Clerk", 20000);
    LevelApprover manager("Manager", 100000);
    clerk.setSuccessor(&manager);
    double amount = 50000;

    Stage* root = new Stage("Review");
    WorkItem* raw = new WorkItem("Authorise repair", "Authorise repair");
    raw->setRole("Assessor");
    root->add(new AuditDecorator(new ApprovalDecorator(raw, &clerk, [&amount]() { return amount; })));

    WorkflowDefinition* def = new WorkflowDefinition("decorated-claim", 1);
    def->setRoot(root);
    def->markValidated();
    facade.registerDefinition(def);

    std::string id = facade.startWorkflow("decorated-claim", DataMap());
    WorkflowInstance* instance = engine.findInstance(id);
    REQUIRE(instance != 0);

    ItemFinder finder("Authorise repair");
    instance->accept(finder);
    WorkItem* item = finder.found;
    REQUIRE(item != 0);
    REQUIRE(finder.layers.size() == 2);
    AuditDecorator* auditLayer = dynamic_cast<AuditDecorator*>(finder.layers[0]);
    ApprovalDecorator* approvalLayer = dynamic_cast<ApprovalDecorator*>(finder.layers[1]);
    REQUIRE(auditLayer != 0);
    REQUIRE(approvalLayer != 0);

    CHECK(item->getStateName() == "Available");

    item->assign();
    auditLayer->start();
    CHECK(auditLayer->history().size() == 1);
    CHECK(item->getStateName() == "InProgress");

    auditLayer->complete("authorised");

    CHECK(approvalLayer->lastOutcome() == APPROVED);
    CHECK(approvalLayer->approvedBy() == "Manager");
    CHECK(item->isComplete());
    CHECK(auditLayer->history().size() == 2);

    BottleneckVisitor report;
    instance->accept(report);
    CHECK(report.getReport().find("[BOTTLENECK]") == std::string::npos);

    ProcessDataStore store;
    store.append(instance->createMemento());
    store.accept(report);
    REQUIRE(store.getLog().size() == 1);
    CHECK(store.getLog()[0]->getStateSnapshot() == "SNAPSHOT");

    CHECK(facade.getInstanceStatus(id) == "RUNNING");
}