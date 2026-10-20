#pragma once
#include "tividy/builder/WorkflowDefinition.h"
/** @file Builder.h  Builder + Director. STUB */
class WorkflowBuilder {
public:
    virtual ~WorkflowBuilder() {}
    virtual void buildStage(const std::string& name, ExecutionMode m) = 0;
    virtual void buildWorkItem(const std::string& stage, const std::string& name) = 0;
    virtual void buildDependency(const std::string& from, const std::string& to) = 0;
    virtual void buildRule(const std::string& key, const std::string& value) = 0;
    virtual WorkflowDefinition* getResult() = 0;
};
class ValidatingWorkflowBuilder : public WorkflowBuilder {
public:
    void buildStage(const std::string&, ExecutionMode) {} void buildWorkItem(const std::string&, const std::string&) {}
    void buildDependency(const std::string&, const std::string&) {} void buildRule(const std::string&, const std::string&) {}
    WorkflowDefinition* getResult() { return 0; }  // TODO
};
class DraftWorkflowBuilder : public WorkflowBuilder {
public:
    void buildStage(const std::string&, ExecutionMode) {} void buildWorkItem(const std::string&, const std::string&) {}
    void buildDependency(const std::string&, const std::string&) {} void buildRule(const std::string&, const std::string&) {}
    WorkflowDefinition* getResult() { return 0; }  // TODO
};
class WorkflowDirector {
public:
    void construct(WorkflowBuilder&) {}  // TODO: call build steps in valid order
};
