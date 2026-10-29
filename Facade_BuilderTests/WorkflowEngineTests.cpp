#include "doctest.h"

#include "tividy/command/WorkflowEngine.h"
#include "tividy/core/WorkflowInstance.h"

TEST_CASE("WorkflowEngine rejects a null instance") {
    WorkflowEngine engine;

    CHECK_FALSE(engine.registerInstance(nullptr));
    CHECK(engine.instanceCount() == 0);
}

TEST_CASE("WorkflowEngine registers an instance") {
    WorkflowEngine engine;

    WorkflowInstance* instance =
        new WorkflowInstance("instance-1", nullptr, DataMap());

    REQUIRE(engine.registerInstance(instance));

    CHECK(engine.instanceCount() == 1);
    CHECK(engine.findInstance("instance-1") == instance);
    CHECK(engine.getStatus("instance-1") == "REGISTERED");
}

TEST_CASE("WorkflowEngine rejects duplicate instance IDs") {
    WorkflowEngine engine;

    WorkflowInstance* first =
        new WorkflowInstance("instance-1", nullptr, DataMap());

    REQUIRE(engine.registerInstance(first));

    WorkflowInstance* duplicate =
        new WorkflowInstance("instance-1", nullptr, DataMap());

    CHECK_FALSE(engine.registerInstance(duplicate));
    CHECK(engine.instanceCount() == 1);

    // Registration failed, so the caller still owns the duplicate.
    delete duplicate;
}

TEST_CASE("WorkflowEngine reports NOT_FOUND for an unknown instance") {
    WorkflowEngine engine;

    CHECK(engine.getStatus("missing") == "NOT_FOUND");
    CHECK(engine.findInstance("missing") == nullptr);
}

TEST_CASE("WorkflowEngine refuses to start an unregistered instance") {
    WorkflowEngine engine;

    WorkflowInstance* instance =
        new WorkflowInstance("instance-1", nullptr, DataMap());

    CHECK_FALSE(engine.start(instance));

    delete instance;
}