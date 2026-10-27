#pragma once
#include <functional>
#include <string>
#include "tividy/core/Common.h"
#include "tividy/mediator/Mediator.h"
#include "ApprovalRequest.h"

/** @file ApprovalHandler.h
 *  @brief Chain of Responsibility: multi-level approval routing.
 *  Level names, limits and decisions are supplied by the workflow
 *  definition / main.cpp, never by this file. */

/** @brief Handler role. Decides the request if it is within its authority,
 *  otherwise passes it to the successor. */
class ApprovalHandler : public WorkflowColleague {
public:
    /** What the approver at this level decides. Return APPROVED, REJECTED
     *  (and set req.reason) or ESCALATED. */
    typedef std::function<ApprovalOutcome(ApprovalRequest&)> Decision;

    /** Called each time a request moves up a level, e.g. to feed the audit trail. */
    typedef std::function<void(const std::string&)> EscalationListener;

    /** @param maxAmount highest amount this level may approve
     *  @param d         the approver's decision (default: approve) */
    explicit ApprovalHandler(double maxAmount = 0, Decision d = Decision());
    virtual ~ApprovalHandler();

    /** Sets the next handler (not owned; the caller keeps it alive).
     *  @return the successor, so calls can be chained.
     *  @throws std::invalid_argument if this would make the chain loop back on itself */
    ApprovalHandler* setSuccessor(ApprovalHandler* next);

    /** Registers a listener told whenever this level escalates to its successor. */
    void setEscalationListener(EscalationListener l);

    /** Template method: decide here, or escalate.
     *  @return APPROVED, REJECTED, or ESCALATED if nobody in the chain decided. */
    ApprovalOutcome handleRequest(ApprovalRequest& req);

    /** @return this level's approval limit */
    double getLimit() const;

protected:
    /** Does this level have authority (limit and deadline) for the request? */
    virtual bool canApprove(const ApprovalRequest& req) const;
    /** The approver's decision at this level. */
    virtual ApprovalOutcome decide(ApprovalRequest& req);
    /** Pass to the successor, or return ESCALATED at the end of the chain. */
    virtual ApprovalOutcome escalate(ApprovalRequest& req);
    /** Display name of this level, recorded in approvedBy. */
    virtual std::string levelName() const;

    double limit;
    ApprovalHandler* successor;   ///< not owned
    Decision decision;
    EscalationListener onEscalate;
};