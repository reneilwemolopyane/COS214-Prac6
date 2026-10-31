#include "Scenario.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"

void Scenario::install(WorkflowEngine& engine, DefinitionRepository& repo) {
    ValidatingWorkflowBuilder builder;
    director().construct(builder);
    repo.save(builder.getResult());      // the repository owns the validated definition
    addParticipants(engine);             // the engine owns the participants
    addConnectors(engine.connectors());  // adapters stay owned by the scenario
}
