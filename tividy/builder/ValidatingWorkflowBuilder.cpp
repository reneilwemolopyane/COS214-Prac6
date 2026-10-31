#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include <queue>
#include <stdexcept>

ValidatingWorkflowBuilder::ValidatingWorkflowBuilder() {}

void ValidatingWorkflowBuilder::reset() {

    WorkflowBuilder::reset();
    itemRoles.clear();
    dependencies.clear();
    stageNames.clear();
}

void ValidatingWorkflowBuilder::buildStage(const std::string& name) {

    if (name.empty()) {

        throw std::invalid_argument("Stage name must not be empty");
    }
    if (!stageNames.insert(name).second) 
    {
        throw std::invalid_argument("Duplicate stage name: " + name);
    }
    appendStage(name);
}

void ValidatingWorkflowBuilder::buildWorkItem(const std::string& name, const std::string& role) {
    if (currentStage == nullptr) {
        throw std::logic_error("buildStage must be called before buildWorkItem");
    }
    if (name.empty()) {
        throw std::invalid_argument("Work item name must not be empty");
    }
    if (itemRoles.count(name) != 0) {
        throw std::invalid_argument("Duplicate work item name: " + name);
    }

    itemRoles[name] = role;

    // NEW: Wrap the item based on metadata (Example placeholder)
    WorkItem* item = new WorkItem(name, name);
    item->setRole(role);
    
    // Check if there's a rule like "requires_approval" or "deadline"
    // If so, wrap it:
    // item = new ApprovalDecorator(item, chain, amountProvider);
    // item = new DeadlineDecorator(item, 3600);

    currentStage->add(item);
}

void ValidatingWorkflowBuilder::buildDependency(const std::string& from, const std::string& to){

    if (!exists(from))
    {
        throw std::invalid_argument("Unknown work item in dependency: " + from);
    }

    if (!exists(to)) {

        throw std::invalid_argument("Unknown work item in dependency: " + to);
    }

    if (from == to) {
        
        throw std::invalid_argument("A work item cannot depend on itself: " + from);
    }

    dependencies.push_back(std::make_pair(from, to));

    product->addDependency(from, to);
}

void ValidatingWorkflowBuilder::buildRule(const std::string& name, const std::string& value) {

    if (name.empty()) {
        throw std::invalid_argument("Rule name must not be empty");
    }

    if (product->getRules().count(name) != 0) {
        throw std::invalid_argument("Duplicate rule name: " + name);
    }
    
    product->addRule(name, value);
}


WorkflowDefinition* ValidatingWorkflowBuilder::getResult() {

    if (itemRoles.empty()) {

        throw std::runtime_error("Cannot build: the workflow has no work items");
    }
    if (!allItemsHaveRoles()){

        throw std::runtime_error("Cannot build: every work item needs a role");
    }

    if (hasCircularDependency()){

        throw std::runtime_error("Cannot build: circular dependency between work items");
    }

    product->markValidated();
    
    return release();
}

bool ValidatingWorkflowBuilder::exists(const std::string& name) const{

    return itemRoles.count(name) != 0;
}

bool ValidatingWorkflowBuilder::allItemsHaveRoles() const {

    for (std::map<std::string, std::string>::const_iterator it = itemRoles.begin();

         it != itemRoles.end(); ++it) 
         
         {

        if (it->second.empty()) return false;
    }
    return true;
}

bool ValidatingWorkflowBuilder::hasCircularDependency() const {
    // Kahn's algorithm: if not every node can be removed, there is a cycle.

    std::map<std::string, std::vector<std::string> > next;

    std::map<std::string, int> incoming;

    for (std::size_t i = 0; i < dependencies.size(); ++i) {

        const std::string& from = dependencies[i].first;

        const std::string& to = dependencies[i].second;

        next[from].push_back(to);

        incoming.insert(std::make_pair(from, 0));

        ++incoming[to];
    }

    std::queue<std::string> ready;

    for (std::map<std::string, int>::const_iterator it = incoming.begin(); it != incoming.end(); ++it) {

        if (it->second == 0) ready.push(it->first);
    }

    std::size_t removed = 0;

    while (!ready.empty()) {

        std::string node = ready.front();

        ready.pop();

        ++removed;

        const std::vector<std::string>& targets = next[node];

        for (std::size_t i = 0; i < targets.size(); ++i) {
            
            if (--incoming[targets[i]] == 0) ready.push(targets[i]);
        }
    }
    return removed != incoming.size();
}
