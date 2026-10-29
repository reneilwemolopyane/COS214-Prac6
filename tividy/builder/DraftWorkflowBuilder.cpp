#include "tividy/builder/DraftWorkflowBuilder.h"

DraftWorkflowBuilder::DraftWorkflowBuilder() {}

void DraftWorkflowBuilder::buildStage(const std::string& name){
    
    appendStage(name); 

}

void DraftWorkflowBuilder::buildWorkItem(const std::string& name, const std::string& role) {

    appendItem(name, role);
}

void DraftWorkflowBuilder::buildDependency(const std::string& from, const std::string& to) {

    product->addDependency(from, to);
}

void DraftWorkflowBuilder::buildRule(const std::string& name, const std::string& value) {
    
    product->addRule(name, value);
}
WorkflowDefinition* DraftWorkflowBuilder::getResult() { 
    
    return release(); 
}
