#include "WorkflowFacade.h"


WorkflowFacade::WorkflowFacade(DefinitionRepository& r, WorkflowEngine& e) : repo(r), engine(e){}



// (Facade owner): find definition -> createInstance -> register -> start -> return id.
std::string WorkflowFacade::startWorkflow(const std::string&, const DataMap&) {



}

