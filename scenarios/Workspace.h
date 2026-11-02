#ifndef WORKSPACE_H
#define WORKSPACE_H
/**
 * @file Workspace.h
 * @brief One industry wired onto its own engine: repository + engine + facade + scenario.
 *
 * Both industries use the same WorkflowEngine, WorkflowFacade and DefinitionRepository classes;
 * only the Scenario object differs. Each workspace is isolated from the other.
 */
#include <memory>
#include "Scenario.h"
#include "tividy/facade/WorkflowFacade.h"

class Workspace {
public:
    explicit Workspace(Scenario* s) : scenario(s), facade(repo, engine) { scenario->install(engine, repo); }
    Scenario& getScenario() { return *scenario; }
    DefinitionRepository& getRepository() { return repo; }
    WorkflowEngine& getEngine() { return engine; }
    WorkflowFacade& getFacade() { return facade; }
private:
    std::unique_ptr<Scenario> scenario;   // declared first: outlives the engine that points at its adapters
    DefinitionRepository repo;
    WorkflowEngine engine;
    WorkflowFacade facade;
    Workspace(const Workspace&); Workspace& operator=(const Workspace&);
};
#endif
