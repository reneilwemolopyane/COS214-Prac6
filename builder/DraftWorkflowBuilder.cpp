#include "DraftWorkflowBuilder.h"

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

void DraftWorkflowBuilder::buildRule(const Rule& rule) { 
    
    product->addRule(rule); 

}

WorkflowDefinition* DraftWorkflowBuilder::getResult() { 
    
    return release(); 
}
