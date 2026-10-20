#pragma once
#include "tividy/composite_prototype/WorkflowComponent.h"
/** @brief Decorator base + ConcreteDecorators. STUB: all behaviour TODO. */
class WorkItemDecorator : public WorkflowComponent {
public:
    explicit WorkItemDecorator(WorkflowComponent* w) : wrapped(w) {}
    void start() {}  void complete() {}  void cancel() {}        // TODO: forward to wrapped
    bool isComplete() const { return false; }                     // TODO
    std::string getName() const { return ""; }                    // TODO
    void accept(WorkflowVisitor&) {}                              // TODO
protected:
    WorkflowComponent* wrapped;
};
#define TIVIDY_DEC(C) class C : public WorkItemDecorator { public: explicit C(WorkflowComponent* w) : WorkItemDecorator(w) {} \
  WorkflowComponent* clone() const { return 0; } /* TODO */ };
TIVIDY_DEC(ValidationDecorator)
TIVIDY_DEC(PriorityDecorator)
TIVIDY_DEC(SecurityDecorator)
TIVIDY_DEC(AuditDecorator)
#undef TIVIDY_DEC
