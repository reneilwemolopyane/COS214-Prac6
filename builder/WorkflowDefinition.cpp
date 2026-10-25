#include "WorkflowDefinition.h"
#include <stdexcept>

WorkflowDefinition::WorkflowDefinition(const std::string& id, int version)
    : id(id), version(version), validated(false), instanceCounter(0) {}

WorkflowInstance* WorkflowDefinition::createInstance(const DataMap& data) {
    if (!root) {
        throw std::logic_error("Definition '" + id + "' has no structure to instantiate");
    }
    // Prototype: deep copy of the template. clone() returns a raw pointer we now own.
    WorkComponent* root->clone();

    ++instanceCounter;

    std::string instanceId = id + "-" + std::to_string(instanceCounter);

    return WorkflowInstance*(new WorkflowInstance(instanceId, std::move(copy), data, version));
}

std::string WorkflowDefinition::getId() const{ 
    
    return id; 

}

int WorkflowDefinition::getVersion() const{ 
    
    return version; 

}

bool WorkflowDefinition::isValidated() const{ 
    
    return validated; 

}

void WorkflowDefinition::setRoot(WorkComponent* root){ 
    
    //What is move??
    //this->root = std::move(root);

    this->root = root;
}

void WorkflowDefinition::markValidated(){
    
    validated = true; 

}

const WorkComponent* WorkflowDefinition::getRoot() const { 
     //get()??
    //return root.get(); 

    return this->root;
}

void WorkflowDefinition::addDependency(const std::string& from, const std::string& to){

    dependencies.push_back(std::make_pair(from, to));

}

const std::vector<std::pair<std::string, std::string> >& WorkflowDefinition::getDependencies() const {

    return dependencies;
}

void WorkflowDefinition::addRule(const std::string& name, const std::string& value) const{
    
    rules[name] = value;
}

const DataMap& WorkflowDefinition::getRules() const { 
    
    
    return rules;

}
