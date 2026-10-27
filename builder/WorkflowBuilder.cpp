#include "WorkflowBuilder.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"

WorkflowBuilder::WorkflowBuilder()
    : product(nullptr),
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

    Stage* root(new Stage("root"));

    rootStage = root;

    currentStage = nullptr;

    product = new WorkflowDefinition(id, version);

    product->setRoot(std::move(root));
}

void WorkflowBuilder::appendStage(const std::string& name) {

    Stage* stage(new Stage(name));

    currentStage = stage;

    rootStage->add(stage);
}

void WorkflowBuilder::appendItem(const std::string& name, const std::string& role) {

    if (currentStage == nullptr) 
    {
        appendStage("Default");
    }
    currentStage->add(new WorkItem(name, role));
}

WorkflowDefinition* WorkflowBuilder::release() {

    WorkflowDefinition* finished = product;
    product = nullptr;

    reset();

    return finished;
}