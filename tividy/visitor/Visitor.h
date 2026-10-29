#ifndef VISITOR_H
#define VISITOR_H

#include <string>

#include "tividy/visitor/WorkflowElement.h"

class WorkflowInstance;
class Stage;
class WorkItem;
class ProcessDataStore;
class WorkflowComponent;

class WorkflowVisitor {
public:
    virtual ~WorkflowVisitor() {}
    virtual void visitInstance(WorkflowInstance& instance) = 0;
    virtual void visitStage(Stage& stage) = 0;
    virtual void visitWorkItem(WorkItem& item) = 0;
    virtual void visitProcessDataStore(ProcessDataStore& data) {}
    /** Called once for every decorator layer wrapped around a work item.
     *  Default does nothing, so existing visitors keep working. */
    virtual void visitDecorator(WorkflowComponent& component) {}
    virtual std::string getReport() const = 0;
};

#endif