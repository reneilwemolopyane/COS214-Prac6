#include "doctest.h"
#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/core/Common.h"
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/core/WorkflowInstance.h"

namespace {
struct CountingComponent : public WorkflowComponent {
    static int alive;
    static int clones;
    CountingComponent() { ++alive; }
    CountingComponent(const CountingComponent&) { ++alive; }
    ~CountingComponent() { --alive; }
    WorkflowComponent* clone() const override { ++clones; return new CountingComponent(*this); }
};
int CountingComponent::alive = 0;
int CountingComponent::clones = 0;
}

TEST_CASE("definition keeps its id and version") {
    WorkflowDefinition def("motor-claim", 3);
    CHECK(def.getId() == "motor-claim");
    CHECK(def.getVersion() == 3);
}

TEST_CASE("a new definition is not validated until marked") {
    WorkflowDefinition def("x", 1);
    CHECK_FALSE(def.isValidated());
    def.markValidated();
    CHECK(def.isValidated());
}

TEST_CASE("createInstance clones the root, and objects are cleaned up") {
    CountingComponent::alive = 0;
    CountingComponent::clones = 0;
    {
        WorkflowDefinition def("x", 1);
        def.setRoot(new CountingComponent());
        WorkflowInstance* instance = def.createInstance(DataMap());

        CHECK(CountingComponent::clones == 1);
        CHECK(CountingComponent::alive == 2);

        delete instance;
        CHECK(CountingComponent::alive == 1);
    }
    CHECK(CountingComponent::alive == 0);
}

#include <stdexcept>

TEST_CASE("rules are stored and a repeated name overwrites") {
    WorkflowDefinition def("x", 1);
    def.addRule("approval.threshold", "20000");
    CHECK(def.getRules().at("approval.threshold") == "20000");

    def.addRule("approval.threshold", "100000");
    CHECK(def.getRules().at("approval.threshold") == "100000");
    CHECK(def.getRules().size() == 1);
}

TEST_CASE("createInstance without a root throws") {
    WorkflowDefinition def("x", 1);
    CHECK_THROWS_AS(def.createInstance(DataMap()), std::logic_error);
}

TEST_CASE("two instances are separate copies") {
    CountingComponent::alive = 0;
    CountingComponent::clones = 0;
    {
        WorkflowDefinition def("x", 1);
        def.setRoot(new CountingComponent());
        WorkflowInstance* a = def.createInstance(DataMap());
        WorkflowInstance* b = def.createInstance(DataMap());

        CHECK(a != b);
        CHECK(CountingComponent::clones == 2);
        CHECK(CountingComponent::alive == 3);   // template + two copies

        delete a;
        delete b;
        CHECK(CountingComponent::alive == 1);
    }
    CHECK(CountingComponent::alive == 0);
}
