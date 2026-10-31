#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/core/WorkflowInstance.h"
#include <stdexcept>

WorkflowDefinition::WorkflowDefinition(const std::string& id, int version)
    : id(id),
      version(version),
      validated(false),
      root(nullptr),
      instanceCounter(0) {
}

WorkflowInstance* WorkflowDefinition::createInstance(const DataMap& data) {

    if (!root) {
        throw std::logic_error(
            "Definition '" + id + "' has no structure to instantiate"
        );
    }

    // Deep copy of the template
    WorkflowComponent* copy = root->clone();

    ++instanceCounter;

    std::string instanceId = id + "-" + std::to_string(instanceCounter);

    WorkflowInstance* instance = new WorkflowInstance(instanceId, copy, data, version);

    {
        std::vector<std::pair<std::string, std::string> > implied = getImpliedDependencies();
        for (std::size_t i = 0; i < implied.size(); ++i) instance->addOrdering(implied[i].first, implied[i].second);
    }
    instance->setItemConfigs(itemConfigs);   // routing, approval, decorator and service settings

    // Each dependency (from, to) means "to" waits for "from".
    for (std::size_t i = 0; i < dependencies.size(); ++i)
        instance->addDependency(dependencies[i].first, dependencies[i].second);

    return instance;
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

void WorkflowDefinition::setRoot(WorkflowComponent* root){ 
    
    //What is move??
    //this->root = std::move(root);

    this->root = root;
}

void WorkflowDefinition::markValidated(){
    
    validated = true; 

}

const WorkflowComponent* WorkflowDefinition::getRoot() const { 
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

void WorkflowDefinition::addRule(const std::string& name, const std::string& value){
    
    rules[name] = value;
}

std::vector<std::pair<std::string, std::string> > WorkflowDefinition::getImpliedDependencies() const {
    std::vector<std::pair<std::string, std::string> > out;
    if (root) root->collectOrdering(out);
    return out;
}

ItemConfig& WorkflowDefinition::itemConfig(const std::string& itemName) { return itemConfigs[itemName]; }

const std::map<std::string, ItemConfig>& WorkflowDefinition::getItemConfigs() const { return itemConfigs; }

const DataMap& WorkflowDefinition::getRules() const { 
    
    
    return rules;

}

WorkflowDefinition::~WorkflowDefinition() {
    
    delete root;
}
