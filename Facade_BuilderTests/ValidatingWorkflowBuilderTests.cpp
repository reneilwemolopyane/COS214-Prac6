#include "doctest.h"

#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/builder/WorkflowDefinition.h"

TEST_CASE("ValidatingWorkflowBuilder builds a valid workflow") {
    ValidatingWorkflowBuilder builder;
    builder.setIdentity("review-workflow", 1);

    builder.buildStage("Review");
    builder.buildWorkItem("Submit request", "Requester");
    builder.buildWorkItem("Review request", "Manager");
    builder.buildDependency("Submit request", "Review request");
    builder.buildRule("approval", "required");

    WorkflowDefinition* result = builder.getResult();

    REQUIRE(result != nullptr);
    CHECK(result->getId() == "review-workflow");
    CHECK(result->getVersion() == 1);
    CHECK(result->isValidated());

    delete result;
}

TEST_CASE("ValidatingWorkflowBuilder rejects an empty stage name") {
    ValidatingWorkflowBuilder builder;

    CHECK_THROWS(builder.buildStage(""));
}

TEST_CASE("ValidatingWorkflowBuilder rejects duplicate stage names") {
    ValidatingWorkflowBuilder builder;

    builder.buildStage("Review");

    CHECK_THROWS(builder.buildStage("Review"));
}

TEST_CASE("ValidatingWorkflowBuilder rejects an empty work item name") {
    ValidatingWorkflowBuilder builder;
    builder.buildStage("S");
    CHECK_THROWS(builder.buildWorkItem("", "Manager"));
}

TEST_CASE("ValidatingWorkflowBuilder rejects duplicate work item names") {
    ValidatingWorkflowBuilder builder;
    builder.buildStage("S");
    builder.buildWorkItem("Review request", "Manager");

    CHECK_THROWS(builder.buildWorkItem("Review request", "Requester"));
}

TEST_CASE("ValidatingWorkflowBuilder rejects a work item without a role") {
    ValidatingWorkflowBuilder builder;
    builder.buildStage("S");
    builder.buildWorkItem("Review request", "");

    CHECK_THROWS(builder.getResult());
}

TEST_CASE("ValidatingWorkflowBuilder rejects a dependency on an unknown item") {
    ValidatingWorkflowBuilder builder;
    builder.buildStage("S");
    builder.buildWorkItem("Submit request", "Requester");

    CHECK_THROWS(builder.buildDependency("Submit request", "Unknown item"));
}

TEST_CASE("ValidatingWorkflowBuilder rejects a self-dependency") {
    ValidatingWorkflowBuilder builder;
    builder.buildStage("S");
    builder.buildWorkItem("Review request", "Manager");

    CHECK_THROWS(builder.buildDependency("Review request", "Review request"));
}

TEST_CASE("ValidatingWorkflowBuilder rejects circular dependencies") {
    ValidatingWorkflowBuilder builder;
    builder.buildStage("S");
    builder.buildWorkItem("A", "Role A");
    builder.buildWorkItem("B", "Role B");

    builder.buildDependency("A", "B");
    builder.buildDependency("B", "A");

    CHECK_THROWS(builder.getResult());
}

TEST_CASE("ValidatingWorkflowBuilder rejects a workflow with no work items") {
    ValidatingWorkflowBuilder builder;

    CHECK_THROWS(builder.getResult());
}

TEST_CASE("ValidatingWorkflowBuilder resets its validation data") {
    ValidatingWorkflowBuilder builder;

    builder.buildStage("Review");
    builder.buildWorkItem("Review request", "Manager");

    builder.reset();

    CHECK(builder.exists("Review request") == false);
    CHECK(builder.allItemsHaveRoles());
    CHECK(builder.hasCircularDependency() == false);
}