#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "tividy/builder/WorkflowDefinition.h"

TEST_CASE("WorkflowDefinition stores its identity and version") {
    WorkflowDefinition definition("review-workflow", 1);

    CHECK(definition.getId() == "review-workflow");
    CHECK(definition.getVersion() == 1);
}

TEST_CASE("WorkflowDefinition starts as unvalidated") {
    WorkflowDefinition definition("review-workflow", 1);

    CHECK_FALSE(definition.isValidated());

    definition.markValidated();

    CHECK(definition.isValidated());
}

TEST_CASE("WorkflowDefinition stores rules") {
    WorkflowDefinition definition("review-workflow", 1);

    definition.addRule("approval", "manager");

    CHECK(definition.getRules().at("approval") == "manager");
}

TEST_CASE("WorkflowDefinition stores dependencies") {
    WorkflowDefinition definition("review-workflow", 1);

    definition.addDependency("Submit request", "Review request");

    const std::vector<std::pair<std::string, std::string> >& dependencies =
        definition.getDependencies();

    REQUIRE(dependencies.size() == 1);
    CHECK(dependencies[0].first == "Submit request");
    CHECK(dependencies[0].second == "Review request");
}

TEST_CASE("WorkflowDefinition rejects instance creation without a root") {
    WorkflowDefinition definition("review-workflow", 1);
    DataMap data;

    CHECK_THROWS_AS(definition.createInstance(data), std::logic_error);
}