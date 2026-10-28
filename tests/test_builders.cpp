#include <stdexcept>
#include "doctest.h"
#include "tividy/builder/DraftWorkflowBuilder.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"

TEST_CASE("validating builder builds a validated definition") {
    ValidatingWorkflowBuilder b;
    b.buildStage("Intake");
    b.buildWorkItem("Register", "Handler");
    b.buildWorkItem("Verify", "Handler");
    b.buildDependency("Register", "Verify");
    b.buildRule("approval.threshold", "20000");
    WorkflowDefinition* def = b.getResult();
    CHECK(def->isValidated());
    delete def;
}

TEST_CASE("validating builder rejects bad input while building") {
    ValidatingWorkflowBuilder b;
    CHECK_THROWS_AS(b.buildWorkItem("Task", "Role"), std::logic_error);  // no stage yet
    b.buildStage("S");
    CHECK_THROWS_AS(b.buildStage("S"), std::invalid_argument);
    CHECK_THROWS_AS(b.buildStage(""), std::invalid_argument);
    b.buildWorkItem("A", "R");
    CHECK_THROWS_AS(b.buildWorkItem("A", "R"), std::invalid_argument);
    CHECK_THROWS_AS(b.buildDependency("A", "Missing"), std::invalid_argument);
    CHECK_THROWS_AS(b.buildDependency("A", "A"), std::invalid_argument);
    CHECK_THROWS_AS(b.buildRule("", "x"), std::invalid_argument);
}

TEST_CASE("validating builder rejects an invalid finished definition") {
    ValidatingWorkflowBuilder b;
    SUBCASE("work item without a role") {
        b.buildStage("S");
        b.buildWorkItem("A", "");
        CHECK_THROWS_AS(b.getResult(), std::runtime_error);
    }
    SUBCASE("circular dependency") {
        b.buildStage("S");
        b.buildWorkItem("A", "R");
        b.buildWorkItem("B", "R");
        b.buildWorkItem("C", "R");
        b.buildDependency("A", "B");
        b.buildDependency("B", "C");
        b.buildDependency("C", "A");
        CHECK_THROWS_AS(b.getResult(), std::runtime_error);
    }
}

TEST_CASE("fork and join dependencies are not a cycle") {
    ValidatingWorkflowBuilder b;
    b.buildStage("S");
    b.buildWorkItem("Start", "R");
    b.buildWorkItem("Left", "R");
    b.buildWorkItem("Right", "R");
    b.buildWorkItem("Join", "R");
    b.buildDependency("Start", "Left");
    b.buildDependency("Start", "Right");
    b.buildDependency("Left", "Join");
    b.buildDependency("Right", "Join");
    WorkflowDefinition* def = b.getResult();
    CHECK(def->isValidated());
    delete def;
}

TEST_CASE("draft builder accepts incomplete input and is never validated") {
    DraftWorkflowBuilder b;
    b.buildWorkItem("Loose", "");
    b.buildDependency("Loose", "NotBuiltYet");
    WorkflowDefinition* def = b.getResult();
    CHECK_FALSE(def->isValidated());
    delete def;
}
