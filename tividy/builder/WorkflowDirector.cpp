#include "tividy/builder/WorkflowDirector.h"

WorkflowDirector::WorkflowDirector() {}

WorkflowDirector::~WorkflowDirector() {}

void WorkflowDirector::construct(WorkflowBuilder& builder) {

    builder.setIdentity("generic-review", 1);

    builder.buildStage("Review");

    builder.buildWorkItem("Submit request", "Requester");

    builder.buildWorkItem("Review request", "Reviewer");

    builder.buildWorkItem("Record decision", "Reviewer");

    builder.buildDependency("Submit request", "Review request");

    builder.buildDependency("Review request", "Record decision");
    
    builder.buildRule("review.decisionRequired", "true");
}
