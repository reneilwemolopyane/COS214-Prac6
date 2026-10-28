#include <stdexcept>
#include "doctest.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/facade/WorkflowFacade.h"

namespace {
WorkflowDefinition* makeDefinition(const std::string& id, bool validated) {
    WorkflowDefinition* def = new WorkflowDefinition(id, 1);
    def->setRoot(new Stage("root"));
    if (validated) def->markValidated();
    return def;
}
}

TEST_CASE("facade starts a registered definition") {
    DefinitionRepository repo;
    WorkflowEngine engine;
    WorkflowFacade facade(repo, engine);
    facade.registerDefinition(makeDefinition("motor-claim", true));

    std::string id = facade.startWorkflow("motor-claim", DataMap());

    CHECK(!id.empty());
    CHECK(engine.findInstance(id) != 0);
    CHECK(facade.getInstanceStatus(id) == "Running");
}

TEST_CASE("two starts give two different instances") {
    DefinitionRepository repo;
    WorkflowEngine engine;
    WorkflowFacade facade(repo, engine);
    facade.registerDefinition(makeDefinition("motor-claim", true));

    std::string a = facade.startWorkflow("motor-claim", DataMap());
    std::string b = facade.startWorkflow("motor-claim", DataMap());

    CHECK(a != b);
    CHECK(engine.instanceCount() == 2);
}

TEST_CASE("facade rejects invalid requests") {
    DefinitionRepository repo;
    WorkflowEngine engine;
    WorkflowFacade facade(repo, engine);

    CHECK_THROWS_AS(facade.startWorkflow("nope", DataMap()), std::out_of_range);

    facade.registerDefinition(makeDefinition("draft", false));
    CHECK_THROWS_AS(facade.startWorkflow("draft", DataMap()), std::logic_error);
    CHECK(engine.instanceCount() == 0);

    CHECK_THROWS_AS(facade.getInstanceStatus("ghost"), std::out_of_range);
}
