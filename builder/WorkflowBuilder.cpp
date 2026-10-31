#include "WorkflowBuilder.h"
#include <stdexcept>
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"

WorkflowBuilder::WorkflowBuilder()
    : product(nullptr),
      parentStage(nullptr),
      rootStage(nullptr),
      currentStage(nullptr),
      id("unnamed"),
      version(1)
{
    reset();
}

WorkflowBuilder::~WorkflowBuilder() {
    delete product;
}

void WorkflowBuilder::setIdentity(const std::string& id, int version) {
    this->id = id;
    this->version = version;

    reset();
}

void WorkflowBuilder::reset() {
    delete product;          // an abandoned partial product (e.g. after setIdentity) must not leak
    product = nullptr;

    Stage* root(new Stage("root", PARALLEL));

    rootStage = root;

    currentStage = nullptr;
    parentStage = nullptr;

    product = new WorkflowDefinition(id, version);

    product->setRoot(std::move(root));
}

void WorkflowBuilder::appendStage(const std::string& name) {
    parentStage = nullptr;   // a new top-level stage closes any open sub-process

    Stage* stage(new Stage(name, PARALLEL));

    currentStage = stage;

    rootStage->add(stage);
}

void WorkflowBuilder::appendItem(const std::string& name, const std::string& role) {

    if (currentStage == nullptr) 
    {
        appendStage("Default");
    }
    // The item's id is its name: dependencies refer to items by name.
    WorkItem* item = new WorkItem(name, name);
    item->setRole(role);
    currentStage->add(item);
}

void WorkflowBuilder::buildDeadline(const std::string& item, long seconds) { product->itemConfig(item).deadlineSeconds = seconds; }

void WorkflowBuilder::buildSubProcess(const std::string& name, ExecutionMode mode) {
    if (!currentStage) throw std::logic_error("buildSubProcess needs a current stage");
    if (parentStage) throw std::logic_error("sub-processes can be nested one level; call buildEndSubProcess first");
    SubProcess* sub = new SubProcess(name, mode);
    currentStage->add(sub);
    parentStage = currentStage;
    currentStage = sub;
}

void WorkflowBuilder::buildEndSubProcess() {
    if (!parentStage) throw std::logic_error("no sub-process is open");
    currentStage = parentStage;
    parentStage = nullptr;
}

void WorkflowBuilder::buildStageMode(ExecutionMode mode) {
    if (!currentStage) throw std::logic_error("buildStageMode needs a current stage");
    currentStage->setMode(mode);
}
void WorkflowBuilder::buildProcessMode(ExecutionMode mode) { rootStage->setMode(mode); }

void WorkflowBuilder::buildGuard(const std::string& item, const std::string& expression) { product->itemConfig(item).guard = expression; }
void WorkflowBuilder::buildApproval(const std::string& item, const std::string& amountKey, const std::string& levels) {
    ItemConfig& c = product->itemConfig(item); c.approvalAmountKey = amountKey; c.approvalLevels = levels;
}
void WorkflowBuilder::buildDecorations(const std::string& item, const std::string& spec) { product->itemConfig(item).decorations = spec; }
void WorkflowBuilder::buildEscalation(const std::string& item, const std::string& role) { product->itemConfig(item).escalationRole = role; }
void WorkflowBuilder::buildAssignmentRule(const std::string& item, const std::string& rule) { product->itemConfig(item).assignmentRule = rule; }
void WorkflowBuilder::buildService(const std::string& item, const std::string& connector, const std::string& operation,
                                   const std::string& params, const std::string& undoOperation) {
    ItemConfig& c = product->itemConfig(item);
    c.serviceConnector = connector; c.serviceOperation = operation; c.serviceParams = params; c.serviceUndo = undoOperation;
}

WorkflowDefinition* WorkflowBuilder::release() {

    WorkflowDefinition* finished = product;
    product = nullptr;

    reset();

    return finished;
}