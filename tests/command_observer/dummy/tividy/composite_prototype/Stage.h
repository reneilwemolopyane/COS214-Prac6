#pragma once
#include "tividy/composite_prototype/WorkflowComponent.h"
class Stage : public WorkflowComponent {
public:
    std::string getName() const { return "stage"; }
    void accept(WorkflowVisitor& v) { v.visitStage(*this); }
};
