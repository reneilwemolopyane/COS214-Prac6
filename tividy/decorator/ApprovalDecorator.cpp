#include "tividy/decorator/ApprovalDecorator.h"
#include <stdexcept>

ApprovalDecorator::ApprovalDecorator(WorkflowComponent* w, ApprovalHandler* firstLevel,
                                     AmountProvider a, ApprovalOutcome d, Sink s)
    : WorkItemDecorator(w), chain(firstLevel), amount(a),
      defaultDecision(d), sink(s), last(ESCALATED) {
    if (!chain) throw std::invalid_argument("ApprovalDecorator needs an approval chain");
}

bool ApprovalDecorator::approve() {
    ApprovalRequest req;
    req.amount = amount ? amount() : 0.0;
    req.workItemId = wrapped->getName();
    req.deadlinePassed = wrapped->isOverdue();

    ApprovalOutcome result = chain->handleRequest(req);
    if (result == ESCALATED) {            // end of chain, nobody decided
        result = defaultDecision;
        if (req.reason.empty()) req.reason = "No approver decided in time";
    }
    last = result;
    approver = req.approvedBy;

    if (sink) {
        std::string msg = (result == APPROVED) ? std::string("approved")
                                               : std::string("rejected");
        msg += ": " + wrapped->getName();
        if (!approver.empty()) msg += " by " + approver;
        sink(msg);
    }

    if (result == APPROVED) return true;
    wrapped->reject(req.reason.empty() ? std::string("Approval rejected") : req.reason);
    return false;
}

void ApprovalDecorator::complete() {
    if (approve()) wrapped->complete();
}

void ApprovalDecorator::complete(const std::string& outcome) {
    if (approve()) wrapped->complete(outcome);
}

ApprovalOutcome ApprovalDecorator::lastOutcome() const { return last; }
std::string ApprovalDecorator::approvedBy() const { return approver; }

WorkflowComponent* ApprovalDecorator::clone() const {
    return new ApprovalDecorator(wrapped->clone(), chain, amount, defaultDecision, sink);
}