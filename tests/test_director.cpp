#include "doctest.h"
#include "tividy/builder/DraftWorkflowBuilder.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/builder/WorkflowDirector.h"

TEST_CASE("director builds a valid process with the validating builder") {
    ValidatingWorkflowBuilder builder;
    WorkflowDirector director;
    director.construct(builder);
    WorkflowDefinition* def = builder.getResult();
    CHECK(def->isValidated());
    delete def;
}

TEST_CASE("the same director with the draft builder gives an unvalidated draft") {
    DraftWorkflowBuilder builder;
    WorkflowDirector director;
    director.construct(builder);
    WorkflowDefinition* def = builder.getResult();
    CHECK_FALSE(def->isValidated());
    delete def;
}
