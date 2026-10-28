#include "doctest.h"
#include "tividy/facade/DefinitionRepository.h"

TEST_CASE("repository finds what it saved") {
    DefinitionRepository repo;
    WorkflowDefinition* def = new WorkflowDefinition("motor-claim", 1);
    repo.save(def);
    CHECK(repo.find("motor-claim") == def);
    CHECK(repo.find("unknown") == nullptr);
}
