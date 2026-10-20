#include "WorkFlowBuilder.h"
#include "tividy/core/Common.h"

class DraftWorkflowBuilder : public WorkflowBuilder {

public:

    void buildStage(const std::string& name);
    
    void buildWorkItem(const std::string& name, const std::string& role);

    void buildDependency(const std::string& from, const std::string& to);
    
    void buildRule(const std::string&, const std::string&);

    WorkflowDefinition* getResult();

};

