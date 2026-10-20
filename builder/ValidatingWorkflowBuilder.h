#include "WorkFlowBuilder.h"
//#include "tividy/core/Common.h"



class ValidatingWorkflowBuilder : public WorkflowBuilder {
public:
    void buildStage(const std::string&);
    
    void buildWorkItem(const std::string&, const std::string&);

    void buildDependency(const std::string&, const std::string&);
    
    void buildRule(const std::string&, const std::string&);

    WorkflowDefinition* getResult();
};