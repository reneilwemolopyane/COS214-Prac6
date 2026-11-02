#include "doctest.h"

#include "tividy/builder/DraftWorkflowBuilder.h"
#include "tividy/builder/WorkflowDefinition.h"

TEST_CASE("DraftWorkflowBuilder builds a workflow definition") {
    DraftWorkflowBuilder builder;
    builder.setIdentity("review-workflow", 1);

    builder.buildStage("Review");
    builder.buildWorkItem("Review request", "Manager");
    builder.buildDependency("Submit request", "Review request");
    builder.buildRule("approval", "required");

    WorkflowDefinition* result = builder.getResult();

    REQUIRE(result != nullptr);
    CHECK(result->getId() == "review-workflow");
    CHECK(result->getVersion() == 1);

    delete result;
}

TEST_CASE("DraftWorkflowBuilder stores dependencies") {
    DraftWorkflowBuilder builder;
    builder.setIdentity("dependency-test", 1);

    builder.buildDependency("Submit", "Review");

    WorkflowDefinition* result = builder.getResult();

    REQUIRE(result != nullptr);

    const std::vector<std::pair<std::string, std::string> >& dependencies =
        result->getDependencies();

    REQUIRE(dependencies.size() == 1);
    CHECK(dependencies[0].first == "Submit");
    CHECK(dependencies[0].second == "Review");

    delete result;
}

TEST_CASE("DraftWorkflowBuilder stores rules") {
    DraftWorkflowBuilder builder;
    builder.setIdentity("rule-test", 1);

    builder.buildRule("approval", "manager");

    WorkflowDefinition* result = builder.getResult();

    REQUIRE(result != nullptr);
    CHECK(result->getRules().at("approval") == "manager");

    delete result;
}

TEST_CASE("DraftWorkflowBuilder resets after returning its result") {
    DraftWorkflowBuilder builder;
    builder.setIdentity("first-workflow", 1);

    WorkflowDefinition* first = builder.getResult();

    REQUIRE(first != nullptr);
    CHECK(first->getId() == "first-workflow");

    delete first;

    WorkflowDefinition* second = builder.getResult();

    REQUIRE(second != nullptr);
    CHECK(second->getId() == "first-workflow");
    CHECK(second->getDependencies().empty());
    CHECK(second->getRules().empty());

    delete second;
}