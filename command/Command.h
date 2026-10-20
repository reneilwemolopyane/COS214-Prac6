#pragma once
#include <string>
#include "tividy/command/ExternalService.h"
/** @file Command.h  Command pattern. STUB */
class WorkItem; class Participant;

class WorkflowCommand {
public:
    WorkflowCommand() : attempts(0) {}
    virtual ~WorkflowCommand() {}
    virtual bool execute() = 0;
    virtual void undo() = 0;
    std::string issuedBy, timestamp, status; int attempts;
};
class AssignWorkItemCommand : public WorkflowCommand {
public:
    AssignWorkItemCommand(WorkItem* i, Participant* p) : item(i), who(p), previous(0) {}
    bool execute() { return false; }  // TODO
    void undo() {}                   // TODO restore previous
private: WorkItem* item; Participant* who; Participant* previous;
};
class CompleteWorkItemCommand : public WorkflowCommand {
public:
    CompleteWorkItemCommand(WorkItem* i, const std::string& o) : item(i), outcome(o) {}
    bool execute() { return false; }  // TODO
    void undo() {}                   // TODO reopen
private: WorkItem* item; std::string outcome;
};
class InvokeServiceCommand : public WorkflowCommand {
public:
    InvokeServiceCommand(ExternalService* s, const std::string& req) : service(s), request(req) {}
    bool execute() { return false; }  // TODO
    void undo() {}                   // TODO reverse (e.g. refund)
private: ExternalService* service; std::string request, referenceId;
};
