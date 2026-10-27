#include "doctest.h"

#include "tividy/facade/DefinitionRepository.h"
#include "tividy/builder/WorkflowDefinition.h"

TEST_CASE("DefinitionRepository saves and finds a definition") {
    DefinitionRepository repository;
    WorkflowDefinition* definition =
        new WorkflowDefinition("review-workflow", 1);

    repository.save(definition);

    CHECK(repository.find("review-workflow") == definition);
}

TEST_CASE("DefinitionRepository returns null for an unknown ID") {
    DefinitionRepository repository;

    CHECK(repository.find("unknown-workflow") == nullptr);
}

TEST_CASE("DefinitionRepository replaces a definition with the same ID") {
    DefinitionRepository repository;

    WorkflowDefinition* first =
        new WorkflowDefinition("review-workflow", 1);
    repository.save(first);

    WorkflowDefinition* replacement =
        new WorkflowDefinition("review-workflow", 2);
    repository.save(replacement);

    CHECK(repository.find("review-workflow") == replacement);
    CHECK(repository.find("review-workflow")->getVersion() == 2);
}

TEST_CASE("DefinitionRepository rejects a null definition") {
    DefinitionRepository repository;

    CHECK_THROWS_AS(
        repository.save(nullptr),
        std::invalid_argument
    );
}