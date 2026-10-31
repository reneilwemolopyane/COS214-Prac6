#include "EducationBursaryWorkflowDirector.h"

void EducationBursaryWorkflowDirector::construct(WorkflowBuilder& builder) {
    builder.setIdentity("bursary-application", 1);
    builder.buildProcessMode(SEQUENTIAL);

    builder.buildStage("Application");
    builder.buildStageMode(SEQUENTIAL);
    builder.buildWorkItem("Submit application", "Student");
    builder.buildWorkItem("Verify registration", "Registrar's Office");
    builder.buildWorkItem("Collect documents", "Student");

    builder.buildStage("Assessment");
    builder.buildWorkItem("Academic record check", "Registrar's Office");
    builder.buildWorkItem("Means test", "Financial Aid Assessor");
    builder.buildWorkItem("Reference check", "Bursary Officer");
    builder.buildWorkItem("Merit ranking", "Bursary Officer");
    builder.buildWorkItem("Need verification", "Financial Aid Assessor");
    builder.buildWorkItem("Shortlist decision", "Bursary Officer");

    builder.buildStage("Award and payment");
    builder.buildWorkItem("Waitlist applicant", "Bursary Officer");
    builder.buildWorkItem("Approve bursary award", "Faculty Officer");
    builder.buildWorkItem("Issue award letter", "Bursary Officer");
    builder.buildWorkItem("Student accepts award", "Student");
    builder.buildWorkItem("Release bursary payment", "Finance Clerk");
    builder.buildWorkItem("Notify student", "Bursary Officer");
    builder.buildWorkItem("Close application", "Bursary Officer");

    // Academic check, means test and reference check run in parallel.
    builder.buildDependency("Collect documents", "Academic record check");
    builder.buildDependency("Collect documents", "Means test");
    builder.buildDependency("Collect documents", "Reference check");
    builder.buildDependency("Academic record check", "Merit ranking");
    builder.buildDependency("Means test", "Need verification");

    // Shortlisting waits for all three checks; the merit or need route that was not taken counts as done.
    builder.buildDependency("Reference check", "Shortlist decision");
    builder.buildDependency("Merit ranking", "Shortlist decision");
    builder.buildDependency("Need verification", "Shortlist decision");

    builder.buildDependency("Shortlist decision", "Waitlist applicant");
    builder.buildDependency("Shortlist decision", "Approve bursary award");
    builder.buildDependency("Approve bursary award", "Issue award letter");
    builder.buildDependency("Issue award letter", "Student accepts award");
    builder.buildDependency("Student accepts award", "Release bursary payment");
    builder.buildDependency("Release bursary payment", "Notify student");
    builder.buildDependency("Waitlist applicant", "Notify student");
    builder.buildDependency("Notify student", "Close application");

    // ---- Routing decisions (Interpreter guards) ----
    builder.buildGuard("Merit ranking", "bursaryType == merit");
    builder.buildGuard("Need verification", "bursaryType == need");
    builder.buildGuard("Waitlist applicant", "fundBalance < $requestedAmount");
    builder.buildGuard("Approve bursary award", "fundBalance >= $requestedAmount");

    // ---- Approval by amount (Chain of Responsibility) ----
    builder.buildApproval("Approve bursary award", "requestedAmount",
                          "Faculty Officer=10000;Bursary Committee=50000;Dean=150000;Finance Director=*");
    builder.buildDecorations("Approve bursary award", "audit,notification");
    builder.buildDeadline("Approve bursary award", 5 * 24 * 3600);   // each approver has 5 days

    // ---- Extra behaviour (Decorator) ----
    builder.buildDecorations("Collect documents", "validation:idDocument");
    builder.buildDecorations("Release bursary payment", "audit,security:Finance Clerk");

    // ---- Escalation and assignment (Strategy) ----
    builder.buildEscalation("Collect documents", "Bursary Officer");
    builder.buildDeadline("Collect documents", 7 * 24 * 3600);
    builder.buildAssignmentRule("Reference check", "round-robin");

    // ---- External systems (Adapter, stubbed) ----
    builder.buildService("Verify registration", "student-records", "verify", "registry=students;key=$studentNumber");
    builder.buildService("Academic record check", "student-records", "lookup", "registry=results;key=$studentNumber");
    builder.buildService("Means test", "income-verification", "verify", "registry=income;key=$studentNumber");
    builder.buildService("Release bursary payment", "payment", "pay", "account=$bankAccount;amount=$requestedAmount*100");

    builder.buildRule("registration.required", "true");
    builder.buildRule("documents.required", "id,proof-of-income,motivation-letter,reference-letter");
    builder.buildRule("documents.missingEscalationDays", "7");
    builder.buildRule("approval.facultyOfficerLimitZAR", "10000");
    builder.buildRule("approval.committeeLimitZAR", "50000");
    builder.buildRule("approval.deanLimitZAR", "150000");
    builder.buildRule("payment.requiresAcceptedAward", "true");
}
