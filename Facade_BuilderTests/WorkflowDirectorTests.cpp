#include "doctest.h"

#include "tividy/builder/WorkflowDirector.h"
#include "tividy/builder/DraftWorkflowBuilder.h"
#include "tividy/builder/WorkflowDefinition.h"

TEST_CASE("WorkflowDirector constructs a generic review workflow") {
    WorkflowDirector director;
    DraftWorkflowBuilder builder;

    director.construct(builder);

    WorkflowDefinition* result = builder.getResult();

    REQUIRE(result != nullptr);
    CHECK(result->getId() == "generic-review");
    CHECK(result->getVersion() == 1);

    const std::vector<std::pair<std::string, std::string> >& dependencies =
        result->getDependencies();

    CHECK(dependencies.size() == 2);
    CHECK_FALSE(result->getRules().empty());

    delete result;
}