#include "memento/ProcessDataStore.h"
#include "memento/Memento.h"
#include "visitor/Visitor.h"

#include <cassert>
#include <chrono>
#include <iostream>
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

void testMementoStoresSnapshotData() {
    DataMap data;
    data["owner"] = "team";
    const auto beforeCreation = std::chrono::system_clock::now();
    InstanceMemento memento(data, "submitted", 3);

    assert(memento.getVersion() == 3);
    assert(memento.getData().at("owner") == "team");
    assert(memento.getStateSnapshot() == "submitted");
    assert(memento.getTimestamp() >= beforeCreation);
    assert(!memento.getTimestampString().empty());
}

void testVisitorArchivesOldestMementoPastThreshold() {
    ProcessDataStore store(2);
    TestVisitor visitor;
    store.append(makeMemento(1));
    store.append(makeMemento(2));

    store.accept(visitor);
    assert(store.getLog().size() == 2);
    assert(store.getBackupStorage().empty());

    store.append(makeMemento(3));
    store.accept(visitor);

    assert(store.getLog().size() == 2);
    assert(store.getBackupStorage().size() == 1);
    assert(store.getBackupStorage().front()->getVersion() == 1);
    assert(store.getLog().front()->getVersion() == 2);
}

void testUndoRemovesNewestActiveMemento() {
    ProcessDataStore store;
    store.append(makeMemento(1));
    store.append(makeMemento(2));

    InstanceMemento* undone = store.undo();
    assert(undone != nullptr);
    assert(undone->getVersion() == 2);
    assert(store.getLog().size() == 1);
    assert(store.getLog().front()->getVersion() == 1);
    delete undone;
}

} // namespace

int main() {
    testMementoStoresSnapshotData();
    testVisitorArchivesOldestMementoPastThreshold();
    testUndoRemovesNewestActiveMemento();
    std::cout << "All memento and caretaker tests passed.\n";
}