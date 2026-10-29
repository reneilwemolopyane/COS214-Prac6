#include "doctest.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "tividy/facade/WorkflowFacade.h"
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/command/WorkflowEngine.h"
#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/builder/WorkflowDirector.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/builder/DraftWorkflowBuilder.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/memento/Memento.h"
#include "tividy/memento/ProcessDataStore.h"

namespace {

class ItemCollector : public WorkflowVisitor {
public:
    std::vector<WorkItem*> items;
    std::vector<std::string> stages;
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage& s) override { stages.push_back(s.getName()); }
    void visitWorkItem(WorkItem& i) override { items.push_back(&i); }
    std::string getReport() const override { return ""; }
};

std::vector<WorkItem*> itemsOf(WorkflowInstance* instance) {
    ItemCollector c;
    instance->accept(c);
    return c.items;
}

const WorkItem* templateItem(const WorkflowDefinition& def, std::size_t stage, std::size_t item) {
    const Stage* root = dynamic_cast<const Stage*>(def.getRoot());
    const Stage* s = dynamic_cast<const Stage*>(root->childAt(stage));
    return dynamic_cast<const WorkItem*>(s->childAt(item));
}

WorkflowDefinition* buildReview() {
    ValidatingWorkflowBuilder builder;
    WorkflowDirector director;
    director.construct(builder);
    return builder.getResult();
}

DataMap claimData(const std::string& who) {
    DataMap d;
    d["claimant"] = who;
    return d;
}

struct System {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade;
    System() : facade(repository, engine) {}
};

}

TEST_CASE("Facade + Builder: a directed, validated definition can be started") {
    System sys;
    sys.facade.registerDefinition(buildReview());

    std::string id = sys.facade.startWorkflow("generic-review", claimData("Kemi"));

    CHECK(id == "generic-review-1");
    CHECK(sys.facade.getInstanceStatus(id) == "RUNNING");
    CHECK(sys.engine.instanceCount() == 1);
    CHECK(sys.engine.findInstance(id) != nullptr);
}

TEST_CASE("Facade + Builder: a draft definition is stored but refused at start") {
    System sys;
    DraftWorkflowBuilder draft;
    draft.setIdentity("half-done", 1);
    draft.buildWorkItem("Only item", "");
    sys.facade.registerDefinition(draft.getResult());

    CHECK(sys.repository.find("half-done") != nullptr);
    CHECK_THROWS_AS(sys.facade.startWorkflow("half-done", DataMap()), std::logic_error);
    CHECK(sys.engine.instanceCount() == 0);
}

TEST_CASE("Facade + Builder: re-registering an id replaces the old definition") {
    System sys;
    sys.facade.registerDefinition(buildReview());

    ValidatingWorkflowBuilder builder;
    builder.setIdentity("generic-review", 2);
    builder.buildStage("Quick");
    builder.buildWorkItem("Approve", "Manager");
    sys.facade.registerDefinition(builder.getResult());

    std::string id = sys.facade.startWorkflow("generic-review", DataMap());
    CHECK(sys.engine.findInstance(id)->getVersion() == 2);
    CHECK(itemsOf(sys.engine.findInstance(id)).size() == 1);
}

TEST_CASE("Builder + Composite: built items keep the names and roles given to the builder") {
    WorkflowDefinition* def = buildReview();
    const WorkItem* first = templateItem(*def, 0, 0);

    CHECK(first->getId() == "Submit request");
    CHECK(first->getRole() == "Requester");
    delete def;
}

TEST_CASE("Facade + Prototype: every started instance gets its own copy of the template") {
    System sys;
    sys.facade.registerDefinition(buildReview());

    std::string a = sys.facade.startWorkflow("generic-review", claimData("A"));
    std::string b = sys.facade.startWorkflow("generic-review", claimData("B"));
    CHECK(a == "generic-review-1");
    CHECK(b == "generic-review-2");

    std::vector<WorkItem*> itemsA = itemsOf(sys.engine.findInstance(a));
    std::vector<WorkItem*> itemsB = itemsOf(sys.engine.findInstance(b));
    WorkflowDefinition* def = sys.repository.find("generic-review");

    REQUIRE(itemsA.size() == 3);
    REQUIRE(itemsB.size() == 3);

    SUBCASE("same structure") {
        for (std::size_t i = 0; i < 3; ++i)
            CHECK(itemsA[i]->getId() == itemsB[i]->getId());
    }
    SUBCASE("different objects, not shared with the template") {
        for (std::size_t i = 0; i < 3; ++i) {
            CHECK(itemsA[i] != itemsB[i]);
            CHECK(itemsA[i] != templateItem(*def, 0, i));
        }
    }
    SUBCASE("changing one instance leaves the other and the template alone") {
        itemsA[0]->setData("note", "only in A");
        CHECK(itemsA[0]->getData("note") == "only in A");
        CHECK(itemsB[0]->getData("note") == "");
        CHECK(templateItem(*def, 0, 0)->getData("note") == "");
    }
    SUBCASE("starting instances does not start the template") {
        CHECK(templateItem(*def, 0, 0)->getStateName() == "Created");
        CHECK(itemsA[0]->getStateName() == "Available");
    }
}

TEST_CASE("Facade + Builder: dependencies from the builder hold in a running instance") {
    System sys;
    sys.facade.registerDefinition(buildReview());
    std::string id = sys.facade.startWorkflow("generic-review", DataMap());

    std::vector<WorkItem*> items = itemsOf(sys.engine.findInstance(id));
    REQUIRE(items.size() == 3);
    CHECK(items[0]->getStateName() == "Available");
    CHECK(items[1]->getStateName() == "Created");
    CHECK(items[2]->getStateName() == "Created");
}

TEST_CASE("Facade + Memento: a snapshot of a started instance holds its start data") {
    System sys;
    sys.facade.registerDefinition(buildReview());
    std::string id = sys.facade.startWorkflow("generic-review", claimData("Kemi"));
    WorkflowInstance* instance = sys.engine.findInstance(id);

    InstanceMemento* m = instance->createMemento();
    CHECK(m->getData().at("claimant") == "Kemi");
    CHECK(m->getVersion() == instance->getVersion());
    delete m;
}

TEST_CASE("Memento + ProcessDataStore: save, change, undo restores the instance") {
    System sys;
    sys.facade.registerDefinition(buildReview());
    std::string id = sys.facade.startWorkflow("generic-review", claimData("Kemi"));
    WorkflowInstance* instance = sys.engine.findInstance(id);

    ProcessDataStore store;
    store.append(instance->createMemento());

    InstanceMemento changed(claimData("Someone else"));
    instance->restoreFromMemento(changed);
    InstanceMemento* now = instance->createMemento();
    CHECK(now->getData().at("claimant") == "Someone else");
    delete now;

    InstanceMemento* saved = store.undo();
    REQUIRE(saved != nullptr);
    instance->restoreFromMemento(*saved);
    delete saved;

    InstanceMemento* back = instance->createMemento();
    CHECK(back->getData().at("claimant") == "Kemi");
    delete back;
    CHECK(store.undo() == nullptr);
}

TEST_CASE("Prototype + Memento: restoring one instance does not touch another") {
    System sys;
    sys.facade.registerDefinition(buildReview());
    WorkflowInstance* a = sys.engine.findInstance(sys.facade.startWorkflow("generic-review", claimData("A")));
    WorkflowInstance* b = sys.engine.findInstance(sys.facade.startWorkflow("generic-review", claimData("B")));

    a->restoreFromMemento(InstanceMemento(claimData("A changed")));

    InstanceMemento* ma = a->createMemento();
    InstanceMemento* mb = b->createMemento();
    CHECK(ma->getData().at("claimant") == "A changed");
    CHECK(mb->getData().at("claimant") == "B");
    delete ma;
    delete mb;
}

TEST_CASE("Full chain: Director -> Builder -> Facade -> Prototype -> Memento") {
    System sys;

    WorkflowDefinition* def = buildReview();
    REQUIRE(def->isValidated());
    sys.facade.registerDefinition(def);

    std::string id = sys.facade.startWorkflow("generic-review", claimData("Kemi"));
    REQUIRE(sys.facade.getInstanceStatus(id) == "RUNNING");
    WorkflowInstance* instance = sys.engine.findInstance(id);

    std::vector<WorkItem*> items = itemsOf(instance);
    REQUIRE(items.size() == 3);
    CHECK(items[0] != templateItem(*sys.repository.find("generic-review"), 0, 0));

    ProcessDataStore store(2);
    for (int i = 0; i < 3; ++i) store.append(instance->createMemento());
    CHECK(store.IspastThreshold());
    store.backupstorage();
    CHECK(store.getLog().size() == 2);
    CHECK(store.getBackupStorage().size() == 1);

    instance->restoreFromMemento(InstanceMemento(claimData("edited")));
    InstanceMemento* last = store.undo();
    instance->restoreFromMemento(*last);
    delete last;

    InstanceMemento* check = instance->createMemento();
    CHECK(check->getData().at("claimant") == "Kemi");
    delete check;
    CHECK(sys.facade.getInstanceStatus(id) == "RUNNING");
}