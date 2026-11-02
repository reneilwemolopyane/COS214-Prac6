#include "ValidatingWorkflowBuilder.h"
#include <queue>
#include "tividy/interpreter/Expression.h"
#include "tividy/chain_of_responsibility/ApprovalRouter.h"
#include "tividy/decorator/DecoratorFactory.h"
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

    if (itemRoles.count(name) != 0){

        throw std::invalid_argument("Duplicate work item name: " + name);
    }

    itemRoles[name] = role;

    appendItem(name, role);
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


void ValidatingWorkflowBuilder::buildGuard(const std::string& item, const std::string& expression) {
    if (!exists(item)) throw std::invalid_argument("Guard for unknown work item: " + item);
    delete ExpressionParser::parse(expression);          // syntax check only; throws invalid_argument
    WorkflowBuilder::buildGuard(item, expression);
}

void ValidatingWorkflowBuilder::buildApproval(const std::string& item, const std::string& amountKey, const std::string& levels) {
    if (!exists(item)) throw std::invalid_argument("Approval for unknown work item: " + item);
    if (amountKey.empty()) throw std::invalid_argument("Approval for " + item + " needs an amount key");
    delete ApprovalRouter::parse(levels);                // syntax check only; throws invalid_argument
    WorkflowBuilder::buildApproval(item, amountKey, levels);
}

void ValidatingWorkflowBuilder::buildDecorations(const std::string& item, const std::string& spec) {
    if (!exists(item)) throw std::invalid_argument("Decorations for unknown work item: " + item);
    DecoratorFactory::validate(spec);
    WorkflowBuilder::buildDecorations(item, spec);
}

void ValidatingWorkflowBuilder::buildDeadline(const std::string& item, long seconds) {
    if (!exists(item)) throw std::invalid_argument("Deadline for unknown work item: " + item);
    if (seconds <= 0) throw std::invalid_argument("Deadline must be positive: " + item);
    WorkflowBuilder::buildDeadline(item, seconds);
}

void ValidatingWorkflowBuilder::buildSubProcess(const std::string& name, ExecutionMode mode) {
    if (name.empty()) throw std::invalid_argument("Sub-process name must not be empty");
    if (!stageNames.insert(name).second) throw std::invalid_argument("Duplicate stage name: " + name);
    WorkflowBuilder::buildSubProcess(name, mode);
}

void ValidatingWorkflowBuilder::buildEscalation(const std::string& item, const std::string& role) {
    if (!exists(item)) throw std::invalid_argument("Escalation for unknown work item: " + item);
    if (role.empty()) throw std::invalid_argument("Escalation role must not be empty");
    WorkflowBuilder::buildEscalation(item, role);
}

void ValidatingWorkflowBuilder::buildAssignmentRule(const std::string& item, const std::string& rule) {
    if (!exists(item)) throw std::invalid_argument("Assignment rule for unknown work item: " + item);
    if (rule != "role" && rule != "shortest-queue" && rule != "round-robin")
        throw std::invalid_argument("Unknown assignment rule: " + rule);
    WorkflowBuilder::buildAssignmentRule(item, rule);
}

void ValidatingWorkflowBuilder::buildService(const std::string& item, const std::string& connector, const std::string& operation,
                                             const std::string& params, const std::string& undoOperation) {
    if (!exists(item)) throw std::invalid_argument("Service for unknown work item: " + item);
    if (connector.empty() || operation.empty()) throw std::invalid_argument("Service binding needs a connector and an operation");
    WorkflowBuilder::buildService(item, connector, operation, params, undoOperation);
}

WorkflowDefinition* ValidatingWorkflowBuilder::getResult() {

    if (itemRoles.empty()) {

        throw std::runtime_error("Cannot build: the workflow has no work items");
    }
    if (!allItemsHaveRoles()){

        throw std::runtime_error("Cannot build: every work item needs a role");
    }

    {   // ordering implied by SEQUENTIAL stages must not contradict the explicit dependencies
        std::vector<std::pair<std::string, std::string> > implied = product->getImpliedDependencies();
        dependencies.insert(dependencies.end(), implied.begin(), implied.end());
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
