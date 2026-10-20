#pragma once
#include "WorkflowDefinition.h"
//#include "tividy/core/Common.h"
/** @file Builder.h  Builder + Director. STUB */
class WorkflowBuilder {

public:
    virtual ~WorkflowBuilder() {}

    virtual void buildStage(const std::string& name) = 0;

    virtual void buildWorkItem(const std::string& stage, const std::string& name) = 0;

    virtual void buildDependency(const std::string& from, const std::string& to) = 0;

    virtual void buildRule(const std::string& key, const std::string& value) = 0;

    virtual WorkflowDefinition* getResult() = 0;
};


