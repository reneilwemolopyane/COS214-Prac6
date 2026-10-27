#ifndef WORKFLOWVISITOR_H
#define WORKFLOWVISITOR_H

#include <chrono>
#include <iostream>

class WorkItemState;
class ChainOfResponsibilityElement;
class ProcessDataStore;
class WorkflowCommand;

#pragma once
/** @file Visitor.h  Visitor: WorkflowElement + WorkflowVisitor + concrete visitors. STUB */
#include <string>


class WorkflowVisitor {
public:
    virtual ~WorkflowVisitor() {}
    virtual void visitState(const WorkItemState& state) = 0;
    virtual void visitHandler(const ChainOfResponsibilityElement& handler) = 0;
    virtual void visitCommand(const WorkflowCommand& command) = 0;
    virtual void visitProcessDataStore(ProcessDataStore& data) = 0;
};

#endif


