#pragma once
#include <functional>
#include <string>
#include "tividy/decorator/WorkItemDecorator.h"
#include "tividy/chain_of_responsibility/ApprovalHandler.h"

/** @brief Runs the approval chain before the wrapped item may complete.
 *  APPROVED  -> the item completes.
 *  REJECTED  -> the item is rejected with the approver's reason.
 *  ESCALATED (nobody in the chain decided) -> the default decision is applied.
 *  A rejection is a normal workflow outcome, so complete() does not throw.
 *  If the wrapped item is overdue, the first level is skipped and the request
 *  escalates (deadlinePassed). Place this decorator OUTSIDE DeadlineDecorator. */
class ApprovalDecorator : public WorkItemDecorator {
public:
    typedef std::function<double()> AmountProvider;
    typedef std::function<void(const std::string&)> Sink;

    /** @param w            wrapped component (ownership transfers)
     *  @param firstLevel   first handler of the chain (not owned)
     *  @param amount       supplies the value compared with each level's limit
     *  @param defaultDecision applied when the chain ends undecided
     *  @param s            optional sink told each decision (audit/notification) */
    ApprovalDecorator(WorkflowComponent* w, ApprovalHandler* firstLevel,
                      AmountProvider amount,
                      ApprovalOutcome defaultDecision = REJECTED,
                      Sink s = Sink());

    void complete();
    void complete(const std::string& outcome) override;

    /** @return last decision; ESCALATED means no decision has been made yet */
    ApprovalOutcome lastOutcome() const;
    /** @return name of the level that approved (empty if none) */
    std::string approvedBy() const;

    WorkflowComponent* clone() const;

private:
    bool approve();   // true if the item may complete
    ApprovalHandler* chain;   // not owned
    AmountProvider amount;
    ApprovalOutcome defaultDecision;
    Sink sink;
    ApprovalOutcome last;
    std::string approver;
};