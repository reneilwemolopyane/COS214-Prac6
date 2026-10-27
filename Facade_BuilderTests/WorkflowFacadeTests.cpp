#include "doctest.h"

#include "tividy/facade/WorkflowFacade.h"
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/command/WorkflowEngine.h"
#include "tividy/builder/WorkflowDefinition.h"

TEST_CASE("WorkflowFacade rejects an unknown definition") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    DataMap data;

    CHECK_THROWS_AS(
        facade.startWorkflow("unknown-workflow", data),
        std::out_of_range
    );
}

TEST_CASE("WorkflowFacade rejects an unvalidated definition") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    WorkflowDefinition* definition =
        new WorkflowDefinition("review-workflow", 1);

    facade.registerDefinition(definition);

    DataMap data;

    CHECK_THROWS_AS(
        facade.startWorkflow("review-workflow", data),
        std::logic_error
    );
}

TEST_CASE("WorkflowFacade returns NOT_FOUND for an unknown instance") {
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade(repository, engine);

    CHECK(facade.getInstanceStatus("unknown-instance") == "NOT_FOUND");
}