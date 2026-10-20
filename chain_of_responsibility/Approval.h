#pragma once
#include <string>
#include "tividy/core/Common.h"
#include "tividy/mediator/Mediator.h"
/** @file Approval.h  Chain of Responsibility. STUB */
struct ApprovalRequest {
    double amount; std::string workItemId, requester, deadline;
    ApprovalRequest() : amount(0) {}
};
class ApprovalHandler : public WorkflowColleague {
public:
    ApprovalHandler(double limit = 0) : limit(limit), successor(0) {}
    virtual ~ApprovalHandler() {}
    void setSuccessor(ApprovalHandler*) {}   // TODO
    virtual ApprovalOutcome handleRequest(ApprovalRequest&) { return ESCALATED; }  // TODO
protected:
    double limit; ApprovalHandler* successor;
};
class HandlerApprover       : public ApprovalHandler { public: HandlerApprover(double l = 20000) : ApprovalHandler(l) {} };
class ManagerApprover       : public ApprovalHandler { public: ManagerApprover(double l = 100000) : ApprovalHandler(l) {} };
class SeniorManagerApprover : public ApprovalHandler { public: SeniorManagerApprover() : ApprovalHandler(1e18) {} };
