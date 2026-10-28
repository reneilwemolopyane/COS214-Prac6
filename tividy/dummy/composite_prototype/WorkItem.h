#pragma once
#include <string>
#include <stdexcept>
#include "tividy/composite_prototype/WorkflowComponent.h"
#include "tividy/mediator/WorkflowMediator.h"
#include "tividy/core/Participant.h"
class WorkItem : public WorkflowComponent, public WorkflowColleague {
public:
    explicit WorkItem(const std::string& name) : name(name), assignee(0), done(false) {}
    std::string getName() const { return name; }
    std::string getId() const { return name; }
    void accept(WorkflowVisitor& v) { v.visitWorkItem(*this); }
    void assign(Participant* p) {
        if (done) throw std::logic_error("Cannot assign work item '" + name + "' in state Completed");
        assignee = p;
    }
    Participant* getAssignee() const { return assignee; }
    void releaseAssignee() { assignee = 0; }
    void complete(const std::string& result) {
        if (done) throw std::logic_error("Cannot complete work item '" + name + "' in state Completed");
        outcome = result;
        done = true;
    }
    std::string getOutcome() const { return outcome; }
    void reopen() {
        if (!done) throw std::logic_error("Cannot reopen work item '" + name + "'");
        done = false;
        outcome.clear();
    }
    bool isComplete() const { return done; }
private:
    std::string name, outcome;
    Participant* assignee;
    bool done;
};
