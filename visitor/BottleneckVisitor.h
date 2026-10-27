#ifndef BOTTLENECKVISITOR_H
#define BOTTLENECKVISITOR_H

#include "WorkflowVisitor.h"
#include <iostream> 

class BottleneckVisitor : public WorkflowVisitor {
public:
    void visitState(const State& state) override;
    void visitHandler(const Chain& handler) override;
    void visitProcessDataStore(ProcessDataStore& data) override;
    void visitCommand(const Command& command) override;

};

#endif