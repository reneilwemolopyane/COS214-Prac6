#include "ApprovalHandler.h"
#include <stdexcept>

ApprovalHandler::ApprovalHandler(double maxAmount, Decision d)
    : limit(maxAmount), successor(0), decision(d) {}

ApprovalHandler::~ApprovalHandler() {}

ApprovalHandler* ApprovalHandler::setSuccessor(ApprovalHandler* next) {
    // walk the chain from `next`; reaching `this` would create an endless loop
    for (ApprovalHandler* p = next; p != 0; p = p->successor) {
        if (p == this)
            throw std::invalid_argument("Approval chain cannot loop back on itself");
    }
    successor = next;
    return next;
}

void ApprovalHandler::setEscalationListener(EscalationListener l) { onEscalate = l; }

ApprovalOutcome ApprovalHandler::handleRequest(ApprovalRequest& req) {
    if (canApprove(req)) {
        ApprovalOutcome outcome = decide(req);
        if (outcome == APPROVED) req.approvedBy = levelName();
        if (outcome != ESCALATED) return outcome;   // APPROVED or REJECTED ends the chain
    }
    return escalate(req);
}

double ApprovalHandler::getLimit() const { return limit; }

bool ApprovalHandler::canApprove(const ApprovalRequest& req) const {
    return !req.deadlinePassed && req.amount <= limit;
}

ApprovalOutcome ApprovalHandler::decide(ApprovalRequest& req) {
    return decision ? decision(req) : APPROVED;
}

ApprovalOutcome ApprovalHandler::escalate(ApprovalRequest& req) {
    if (!successor) return ESCALATED;     // end of chain: engine applies default decision
    if (onEscalate)
        onEscalate(levelName() + " escalated " + req.workItemId +
                   " to " + successor->levelName());
    req.deadlinePassed = false;           // next level gets a fresh deadline
    return successor->handleRequest(req);
}

std::string ApprovalHandler::levelName() const { return ""; }