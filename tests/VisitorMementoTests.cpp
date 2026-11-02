#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "memento/ProcessDataStore.h"
#include "memento/Memento.h"
#include "visitor/Visitor.h"
#include "core/WorkflowInstance.h"
#include "composite_prototype/Stage.h"
#include "composite_prototype/WorkItem.h"

#include "doctest.h"
#include <chrono>
#include <string>

namespace {

InstanceMemento* makeMemento(int version) {
    return new InstanceMemento(DataMap(), "snapshot", version);
}

class TestVisitor : public WorkflowVisitor {
public:
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    void visitWorkItem(WorkItem&) override {}
    void visitProcessDataStore(ProcessDataStore& store) override {
        if (store.IspastThreshold()) {
            store.backupstorage();
        }
    }
    std::string getReport() const override { return std::string(); }
};

TEST_CASE("Memento stores snapshot data") {
    DataMap data;
    data["owner"] = "team";
    const auto beforeCreation = std::chrono::system_clock::now();
    InstanceMemento memento(data, "submitted", 3);

    CHECK(memento.getVersion() == 3);
    CHECK(memento.getData().at("owner") == "team");
    CHECK(memento.getStateSnapshot() == "submitted");
    CHECK(memento.getTimestamp() >= beforeCreation);
    CHECK(!memento.getTimestampString().empty());
}

TEST_CASE("Visitor archives oldest memento past threshold") {
    ProcessDataStore store(2);
    TestVisitor visitor;
    store.append(makeMemento(1));
    store.append(makeMemento(2));

    store.accept(visitor);
    CHECK(store.getLog().size() == 2);
    CHECK(store.getBackupStorage().empty());

    store.append(makeMemento(3));
    store.accept(visitor);

    CHECK(store.getLog().size() == 2);
    CHECK(store.getBackupStorage().size() == 1);
    CHECK(store.getBackupStorage().front()->getVersion() == 1);
    CHECK(store.getLog().front()->getVersion() == 2);
}

TEST_CASE("Caretaker undo removes newest active memento") {
    ProcessDataStore store;
    store.append(makeMemento(1));
    store.append(makeMemento(2));

    InstanceMemento* undone = store.undo();
    CHECK(undone != nullptr);
    CHECK(undone->getVersion() == 2);
    CHECK(store.getLog().size() == 1);
    CHECK(store.getLog().front()->getVersion() == 1);
    delete undone;
}

TEST_CASE("WorkflowInstance memento restores execution state") {
    Stage* root = new Stage("root");
    WorkItem* item = new WorkItem("review-1", "Review");
    item->setData("decision", "pending");
    root->add(item);
    DataMap instanceData;
    instanceData["case"] = "original";
    WorkflowInstance instance("instance-1", root, instanceData, 7);

    InstanceMemento* saved = instance.createMemento();
    CHECK(saved->getVersion() == 7);
    CHECK(saved->getStateSnapshot().find("review-1=Created") != std::string::npos);

    item->setData("decision", "approved");
    item->makeAvailable();
    instance.restoreFromMemento(*saved);

    CHECK(instance.getVersion() == 7);
    CHECK(item->getStateName() == "Created");
    CHECK(item->getData("decision") == "pending");
    CHECK(item->getOutcome().empty());
    delete saved;
}

} // namespace

