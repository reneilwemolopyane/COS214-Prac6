#include "MotorClaimWorkflowDirector.h"

// Insurance industry: a motor-vehicle accident claim.
// Everything here is configuration for the generic engine: item names, roles, dependencies and
// the text of guards, approval levels, decorators and service bindings. No engine class is changed.
void MotorClaimWorkflowDirector::construct(WorkflowBuilder& builder) {
    builder.setIdentity("motor-claim", 1);
    builder.buildProcessMode(SEQUENTIAL);        // Composite: each stage starts after the previous one finished

    builder.buildStage("Claim intake");
    builder.buildStageMode(SEQUENTIAL);          // lodge -> verify -> collect, ordered by the stage itself
    builder.buildWorkItem("Lodge claim", "Policyholder");
    builder.buildWorkItem("Verify policy coverage", "Claims Handler");
    builder.buildWorkItem("Collect claim documents", "Policyholder");

    builder.buildStage("Assessment and investigation");
    builder.buildStageMode(PARALLEL); 
    builder.buildWorkItem("Verify police report", "Claims Handler");
    builder.buildWorkItem("Assess vehicle damage", "Assessor");
    builder.buildWorkItem("Obtain repair estimate", "Panel Beater");
    builder.buildWorkItem("Screen claim for fraud", "Fraud Investigator");
    builder.buildWorkItem("Fraud investigation", "Fraud Investigator");

    builder.buildStage("Decision and settlement");
    //builder.buildStageMode(SEQUENTIAL);
    builder.buildWorkItem("Review claim and decide", "Claims Handler");
    builder.buildWorkItem("Approve settlement amount", "Claims Manager");
    builder.buildWorkItem("Authorise write-off payout", "Claims Handler");
    builder.buildSubProcess("Repair", SEQUENTIAL);   // nested Composite: its own ordered group
    builder.buildWorkItem("Authorise repair", "Claims Handler");
    builder.buildWorkItem("Repair vehicle", "Panel Beater");
    builder.buildWorkItem("Inspect completed repairs", "Assessor");
    builder.buildEndSubProcess();
    builder.buildWorkItem("Release settlement payment", "Finance Clerk");
    builder.buildWorkItem("Close claim", "Claims Handler");

    // These checks can proceed in parallel once the documents are collected.
    builder.buildDependency("Collect claim documents", "Verify police report");
    builder.buildDependency("Collect claim documents", "Assess vehicle damage");
    builder.buildDependency("Collect claim documents", "Obtain repair estimate");
    builder.buildDependency("Collect claim documents", "Screen claim for fraud");
    builder.buildDependency("Screen claim for fraud", "Fraud investigation");

    // The claim decision waits for every parallel check (a skipped investigation counts as done).
    builder.buildDependency("Verify police report", "Review claim and decide");
    builder.buildDependency("Assess vehicle damage", "Review claim and decide");
    builder.buildDependency("Obtain repair estimate", "Review claim and decide");
    builder.buildDependency("Screen claim for fraud", "Review claim and decide");
    builder.buildDependency("Fraud investigation", "Review claim and decide");

    builder.buildDependency("Review claim and decide", "Approve settlement amount");
    builder.buildDependency("Approve settlement amount", "Authorise repair");
    builder.buildDependency("Approve settlement amount", "Authorise write-off payout");
    builder.buildDependency("Authorise repair", "Repair vehicle");
    builder.buildDependency("Repair vehicle", "Inspect completed repairs");
    builder.buildDependency("Inspect completed repairs", "Release settlement payment");
    builder.buildDependency("Authorise write-off payout", "Release settlement payment");
    builder.buildDependency("Release settlement payment", "Close claim");

    // ---- Routing decisions (Interpreter guards) ----
    builder.buildGuard("Fraud investigation","Screen_claim_for_fraud_outcome == SUSPICIOUS or fraudScore >= 70");
    builder.buildGuard("Authorise repair","Review_claim_and_decide_outcome == REPAIR");
    builder.buildGuard("Authorise write-off payout","Review_claim_and_decide_outcome == WRITE_OFF");

    // ---- Approval by amount (Chain of Responsibility) ----
    builder.buildApproval("Approve settlement amount", "claimAmount",
                          "Claims Handler=20000;Claims Manager=100000;Senior Manager=*");
    builder.buildDecorations("Approve settlement amount", "priority,audit,notification");   // priority halves the deadline
    builder.buildDeadline("Approve settlement amount", 2 * 24 * 3600);   // each level has 2 days (1 with priority)

    // ---- Extra behaviour (Decorator) ----
    builder.buildDecorations("Collect claim documents", "validation:policeCaseNumber");
    builder.buildDecorations("Fraud investigation", "priority,audit");
    builder.buildDecorations("Release settlement payment", "audit,security:Finance Clerk");

    // ---- Who takes over when nobody responds (escalation) and how work is shared (Strategy) ----
    builder.buildEscalation("Collect claim documents", "Claims Handler");
    builder.buildDeadline("Collect claim documents", 7 * 24 * 3600);     // 7 days without a response
    builder.buildAssignmentRule("Verify police report", "shortest-queue");

    // ---- External systems (Adapter) ----
    builder.buildService("Verify policy coverage", "policy-admin", "verify", "registry=policies;key=$policyNumber");
    builder.buildService("Verify police report", "police-system", "verify", "registry=cases;key=$policeCaseNumber");
    builder.buildService("Release settlement payment", "payment", "pay", "account=$bankAccount;amount=$claimAmount*100");

    builder.buildRule("policy.activeRequired", "true");
    builder.buildRule("documents.required", "police-report,driver-licence,accident-photos,repair-estimate");
    builder.buildRule("documents.missingEscalationDays", "7");
    builder.buildRule("assessment.checksRunInParallel", "true");
    builder.buildRule("fraud.suspiciousClaimsRequireInvestigation", "true");
    builder.buildRule("approval.handlerLimitZAR", "20000");
    builder.buildRule("approval.managerLimitZAR", "100000");
    builder.buildRule("payment.requiresPassedFinalInspection", "true");
    builder.buildRule("settlement.writeOffAlternative", "cash-payout");
}
