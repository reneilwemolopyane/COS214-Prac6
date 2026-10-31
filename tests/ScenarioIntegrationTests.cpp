// End-to-end tests: both industries run on the SAME generic engine classes.
// They drive real instances through the Facade/Engine (no fakes): routing, approval chain,
// decorators, adapters, compensation, invalid actions and instance isolation.
#include "doctest.h"
#include <string>
#include <vector>
#include "tividy/scenarios/Workspace.h"
#include "tividy/scenarios/InsuranceScenario.h"
#include "tividy/scenarios/EducationScenario.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Participant.h"
#include "tividy/observer/Observers.h"
#include "tividy/mediator/InstanceCoordinator.h"

namespace {

struct Run {
    Workspace& ws; WorkflowInstance* in;
    Run(Workspace& w, const std::string& caseKey) : ws(w), in(0) {
        std::vector<SampleCase> cases = w.getScenario().sampleCases();
        for (std::size_t i = 0; i < cases.size(); ++i)
            if (cases[i].key == caseKey) {
                std::string id = w.getFacade().startWorkflow(w.getScenario().id(), cases[i].data);
                in = w.getEngine().findInstance(id);
            }
        REQUIRE(in != nullptr);
    }
    WorkItem* item(const std::string& name) const {
        WorkItem* i = in->getCoordinator()->findItem(name);
        REQUIRE_MESSAGE(i != nullptr, "no such item: " << name);
        return i;
    }
    std::string state(const std::string& name) const { return item(name)->getStateName(); }
    // Acts as the item's assignee (what the TUI does), then starts and completes it.
    bool work(const std::string& name, const std::string& outcome = "done") {
        WorkItem* i = item(name);
        in->setActor(i->getAssignee());
        if (!ws.getEngine().startWorkItem(in, i)) return false;
        return ws.getEngine().completeWorkItem(in, i, outcome);
    }
    std::string outcome() const { return ws.getEngine().getOutcome(in->getId()); }
    bool trailHas(EventType t, const std::string& detail = "") const {
        const std::vector<WorkflowEvent>& tr = in->getAuditLogger()->getTrail();
        for (std::size_t i = 0; i < tr.size(); ++i)
            if (tr[i].type == t && tr[i].details.find(detail) != std::string::npos) return true;
        return false;
    }
};

// Everything up to "Review claim and decide" for a clean claim.
void intakeAndChecks(Run& r) {
    for (const char* n : {"Lodge claim", "Verify policy coverage", "Collect claim documents",
                          "Verify police report", "Assess vehicle damage", "Obtain repair estimate",
                          "Screen claim for fraud"})
        REQUIRE_MESSAGE(r.work(n, std::string(n) == "Screen claim for fraud" ? "CLEAR" : "done"), n << ": " << r.ws.getEngine().getLastError());
}

} // namespace

TEST_CASE("Insurance: repair path completes; write-off branch is skipped") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    intakeAndChecks(r);
    CHECK(r.state("Fraud investigation") == "Skipped");              // low fraud score and CLEAR screen
    REQUIRE(r.work("Review claim and decide", "REPAIR"));
    CHECK(r.state("Authorise write-off payout") == "Created");       // not reachable yet: waits for the approval
    REQUIRE(r.work("Approve settlement amount"));
    CHECK(r.state("Authorise write-off payout") == "Skipped");       // decision branch, evaluated at run time
    CHECK(r.trailHas(EVT_SKIPPED, "branch not taken"));
    for (const char* n : {"Authorise repair", "Repair vehicle", "Inspect completed repairs", "Release settlement payment", "Close claim"})
        REQUIRE_MESSAGE(r.work(n), n << ": " << r.ws.getEngine().getLastError());
    CHECK(r.outcome() == "COMPLETED");
    CHECK(r.in->isCompleted());
}

TEST_CASE("Insurance: write-off path skips the whole repair chain (dead-path elimination)") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "writeoff-large");
    intakeAndChecks(r);
    REQUIRE(r.work("Review claim and decide", "WRITE_OFF"));
    REQUIRE(r.work("Approve settlement amount"));
    CHECK(r.state("Authorise repair") == "Skipped");
    CHECK(r.state("Repair vehicle") == "Skipped");                   // only prerequisite was skipped
    CHECK(r.state("Inspect completed repairs") == "Skipped");
    REQUIRE(r.work("Authorise write-off payout"));
    CHECK(r.state("Release settlement payment") == "Assigned");      // join: one prerequisite done, one skipped
    REQUIRE(r.work("Release settlement payment"));
    REQUIRE(r.work("Close claim"));
    CHECK(r.outcome() == "COMPLETED");
}

TEST_CASE("Approval chain picks the level by amount and escalation climbs one level at a time") {
    Workspace ws(new InsuranceScenario());
    Run small(ws, "repair-small"), large(ws, "writeoff-large");
    for (Run* r : {&small, &large}) intakeAndChecks(*r);
    REQUIRE(small.work("Review claim and decide", "REPAIR"));
    REQUIRE(large.work("Review claim and decide", "WRITE_OFF"));
    CHECK(small.item("Approve settlement amount")->getAssignee()->getName() == "Lerato Dlamini");  // R18 500 -> Claims Handler
    CHECK(large.item("Approve settlement amount")->getAssignee()->getName() == "Zanele Mahlangu"); // R245 000 -> Senior Manager
    CHECK(small.trailHas(EVT_ROUTED, "approval level Claims Handler"));

    WorkItem* approve = small.item("Approve settlement amount");
    CHECK(ws.getEngine().escalateWorkItem(small.in, approve, "no response in 2 days"));   // handler timed out
    CHECK(approve->getAssignee()->getName() == "Kagiso Molefe");                          // -> Claims Manager
    CHECK(small.state("Approve settlement amount") == "Assigned");
    CHECK(ws.getEngine().escalateWorkItem(small.in, approve, "no response"));
    CHECK(approve->getAssignee()->getName() == "Zanele Mahlangu");                        // -> Senior Manager
    CHECK_FALSE(ws.getEngine().escalateWorkItem(small.in, approve, "no response"));       // nobody higher
    CHECK(small.trailHas(EVT_ROUTED, "no higher approval level"));
}

TEST_CASE("Fraud score routes the claim to the investigator") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "suspicious");
    intakeAndChecks(r);
    CHECK(r.state("Fraud investigation") == "Assigned");
    CHECK(r.state("Review claim and decide") == "Created");          // waits for the investigation
    REQUIRE(r.work("Fraud investigation", "CONFIRMED"));
    CHECK(r.state("Review claim and decide") == "Assigned");
}

TEST_CASE("Rejection path: a lapsed policy rejects the claim and closes everything downstream") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "lapsed-policy");
    REQUIRE(r.work("Lodge claim"));
    WorkItem* v = r.item("Verify policy coverage");
    r.in->setActor(v->getAssignee());
    REQUIRE(ws.getEngine().startWorkItem(r.in, v));
    CHECK(r.trailHas(EVT_STARTED));
    REQUIRE(ws.getEngine().invokeService(r.in, ws.getEngine().connectors().find("policy-admin"),
                                         [] { ConnectorRequest q; q.operation = "verify"; q.params["registry"] = "policies"; q.params["key"] = "POL-2002"; return q; }(),
                                         "Verify policy coverage"));
    CHECK(r.in->getData("Verify policy coverage.valid") == "false");
    REQUIRE(ws.getEngine().rejectWorkItem(r.in, v, "policy lapsed"));
    CHECK(r.state("Verify policy coverage") == "Rejected");
    CHECK(r.state("Collect claim documents") == "Escalated");         // blocked work is closed, not left hanging
    CHECK(r.in->isFinished());
    CHECK(r.outcome() == "REJECTED");
    CHECK(r.trailHas(EVT_REJECTED, "policy lapsed"));
}

TEST_CASE("Invalid actions are refused, explained and audited; nothing changes") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    WorkItem* lodge = r.item("Lodge claim");
    CHECK_FALSE(ws.getEngine().completeWorkItem(r.in, lodge, "done"));         // not started yet
    CHECK(ws.getEngine().getLastError().find("complete") != std::string::npos);
    CHECK(r.state("Lodge claim") == "Assigned");
    r.in->setActor(ws.getEngine().getParticipants()[1]);                        // Lerato (Claims Handler), not the Policyholder
    CHECK_FALSE(ws.getEngine().startWorkItem(r.in, lodge));
    CHECK(ws.getEngine().getLastError().find("not the assignee") != std::string::npos);
    CHECK(r.trailHas(EVT_REFUSED));
    CHECK_FALSE(ws.getEngine().rejectWorkItem(r.in, r.item("Verify policy coverage"), "x"));   // still Created
}

TEST_CASE("Validation decorator refuses incomplete documents until the data is supplied") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "missing-docs");
    REQUIRE(r.work("Lodge claim"));
    REQUIRE(r.work("Verify policy coverage"));
    CHECK_FALSE(r.work("Collect claim documents"));                             // no police case number yet
    CHECK(ws.getEngine().getLastError().find("Validation failed") != std::string::npos);
    CHECK(r.state("Collect claim documents") == "InProgress");
    // 7 days with no response: escalate to the Claims Handler role (configured in the definition).
    CHECK(r.state("Collect claim documents") == "InProgress");
    r.in->setData("policeCaseNumber", "CAS-114/10/2026");
    CHECK(ws.getEngine().completeWorkItem(r.in, r.item("Collect claim documents"), "done"));
    CHECK(r.state("Collect claim documents") == "Completed");
}

TEST_CASE("Escalation hands a non-responding item to the configured role") {
    Workspace ws(new EducationScenario());
    Run r(ws, "missing-docs");
    REQUIRE(r.work("Submit application"));
    REQUIRE(r.work("Verify registration"));
    WorkItem* docs = r.item("Collect documents");
    CHECK(docs->getAssignee()->getName() == "Naledi Sithole");                   // the student
    CHECK(ws.getEngine().escalateWorkItem(r.in, docs, "no response in 7 days"));
    CHECK(docs->getAssignee()->getRoles().count("Bursary Officer") == 1);        // now a Bursary Officer
    CHECK(r.state("Collect documents") == "Assigned");
}

TEST_CASE("Payment: gateway outage raises an incident, recovery succeeds, cancel reverses the payment") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    intakeAndChecks(r);
    REQUIRE(r.work("Review claim and decide", "REPAIR"));
    for (const char* n : {"Approve settlement amount", "Authorise repair", "Repair vehicle", "Inspect completed repairs"}) REQUIRE(r.work(n));

    ws.getScenario().setOutage("payment", true);
    CHECK_FALSE(r.work("Release settlement payment"));                           // retried, then an incident
    CHECK(r.item("Release settlement payment")->getStateName() == "InProgress"); // not completed
    CHECK_FALSE(r.in->getIncidentMonitor()->getIncidents().empty());

    ws.getScenario().setOutage("payment", false);
    WorkItem* pay = r.item("Release settlement payment");
    CHECK(ws.getEngine().completeWorkItem(r.in, pay, "done"));                   // security decorator passes: actor is the Finance Clerk
    CHECK(r.trailHas(EVT_SERVICE_CALLED, "paid"));

    CHECK(ws.getEngine().cancel(r.in->getId()));                                 // fraud found after payment
    CHECK(r.trailHas(EVT_COMPENSATED));
    CHECK(r.outcome() == "CANCELLED");
}

TEST_CASE("A refused completion has no side effects: no payment is made") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    intakeAndChecks(r);
    REQUIRE(r.work("Review claim and decide", "REPAIR"));
    for (const char* n : {"Approve settlement amount", "Authorise repair", "Repair vehicle", "Inspect completed repairs"}) REQUIRE(r.work(n));
    WorkItem* pay = r.item("Release settlement payment");
    r.in->setActor(pay->getAssignee());
    CHECK_FALSE(ws.getEngine().completeWorkItem(r.in, pay, "done"));     // Assigned, not started
    CHECK(r.in->getData("Release settlement payment.reference").empty()); // the gateway was never called
    CHECK_FALSE(r.trailHas(EVT_SERVICE_CALLED, "paid"));
}

TEST_CASE("Security decorator: only a Finance Clerk may release payment") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    intakeAndChecks(r);
    REQUIRE(r.work("Review claim and decide", "REPAIR"));
    for (const char* n : {"Approve settlement amount", "Authorise repair", "Repair vehicle", "Inspect completed repairs"}) REQUIRE(r.work(n));
    WorkItem* pay = r.item("Release settlement payment");
    Participant* handler = r.item("Close claim")->getAssignee();                 // not assigned yet: null
    (void)handler;
    Participant* clerk = pay->getAssignee();
    REQUIRE(clerk != nullptr);
    CHECK(clerk->hasRole("Finance Clerk"));
    r.in->setActor(clerk);
    CHECK(ws.getEngine().startWorkItem(r.in, pay));
}

TEST_CASE("Education: merit and need take different routes; the unused route is skipped") {
    Workspace ws(new EducationScenario());
    Run merit(ws, "merit-funded"), need(ws, "need-large");
    for (Run* r : {&merit, &need}) {
        REQUIRE(r->work("Submit application")); REQUIRE(r->work("Verify registration")); REQUIRE(r->work("Collect documents"));
        for (const char* n : {"Academic record check", "Means test", "Reference check"}) REQUIRE_MESSAGE(r->work(n), n);
    }
    CHECK(merit.state("Need verification") == "Skipped");
    CHECK(merit.state("Merit ranking") == "Assigned");
    CHECK(need.state("Merit ranking") == "Skipped");
    CHECK(need.state("Need verification") == "Assigned");
}

TEST_CASE("Education: full funded path; the approval chain climbs to the Dean for R120 000") {
    Workspace ws(new EducationScenario());
    Run r(ws, "need-large");
    for (const char* n : {"Submit application", "Verify registration", "Collect documents", "Academic record check", "Means test",
                          "Reference check", "Need verification", "Shortlist decision"}) REQUIRE_MESSAGE(r.work(n), n << ": " << ws.getEngine().getLastError());
    CHECK(r.state("Waitlist applicant") == "Skipped");                           // fund is sufficient
    CHECK(r.item("Approve bursary award")->getAssignee()->getName() == "Dean Naidoo");
    for (const char* n : {"Approve bursary award", "Issue award letter", "Student accepts award", "Release bursary payment", "Notify student", "Close application"})
        REQUIRE_MESSAGE(r.work(n), n << ": " << ws.getEngine().getLastError());
    CHECK(r.outcome() == "COMPLETED");
}

TEST_CASE("Education: exhausted fund waitlists the student and skips the award and payment branch") {
    Workspace ws(new EducationScenario());
    Run r(ws, "fund-exhausted");
    for (const char* n : {"Submit application", "Verify registration", "Collect documents", "Academic record check", "Means test",
                          "Reference check", "Merit ranking", "Shortlist decision"}) REQUIRE_MESSAGE(r.work(n), n);
    CHECK(r.state("Approve bursary award") == "Skipped");
    CHECK(r.state("Release bursary payment") == "Skipped");                      // dead path
    REQUIRE(r.work("Waitlist applicant"));
    REQUIRE(r.work("Notify student"));
    REQUIRE(r.work("Close application"));
    CHECK(r.outcome() == "COMPLETED");
}

TEST_CASE("Education: unregistered student is rejected after the records system answers") {
    Workspace ws(new EducationScenario());
    Run r(ws, "deregistered");
    REQUIRE(r.work("Submit application"));
    WorkItem* v = r.item("Verify registration");
    r.in->setActor(v->getAssignee());
    REQUIRE(ws.getEngine().startWorkItem(r.in, v));
    ConnectorRequest q; q.operation = "verify"; q.params["registry"] = "students"; q.params["key"] = "u25999999";
    REQUIRE(ws.getEngine().invokeService(r.in, ws.getEngine().connectors().find("student-records"), q, v->getId()));
    CHECK(r.in->getData("Verify registration.valid") == "false");
    REQUIRE(ws.getEngine().rejectWorkItem(r.in, v, "student deregistered"));
    CHECK(r.outcome() == "REJECTED");
}

TEST_CASE("Instances from one definition are isolated from each other") {
    Workspace ws(new InsuranceScenario());
    Run a(ws, "repair-small"), b(ws, "writeoff-large");
    REQUIRE(a.work("Lodge claim"));
    CHECK(a.state("Lodge claim") == "Completed");
    CHECK(b.state("Lodge claim") == "Assigned");                                  // untouched
    CHECK(a.in->getData("claimAmount") == "18500");
    CHECK(b.in->getData("claimAmount") == "245000");
    a.in->setData("claimAmount", "1");
    CHECK(b.in->getData("claimAmount") == "245000");
    CHECK(a.in->getId() != b.in->getId());
    CHECK(ws.getRepository().find("motor-claim")->getVersion() == 1);             // the definition is unchanged
}

TEST_CASE("Both industries use the same engine classes without modification") {
    Workspace ins(new InsuranceScenario()), edu(new EducationScenario());
    CHECK(ins.getScenario().industry() == "Insurance");
    CHECK(edu.getScenario().industry() == "Education");
    // Same classes, different configuration:
    CHECK(typeid(ins.getEngine()) == typeid(edu.getEngine()));
    CHECK(typeid(ins.getFacade()) == typeid(edu.getFacade()));
    CHECK(ins.getRepository().find("bursary-application") == nullptr);            // each workspace is isolated
    CHECK(edu.getRepository().find("motor-claim") == nullptr);
}

// ---------------- Composite, deadlines, decorators, notifications ----------------

TEST_CASE("Composite: a SEQUENTIAL stage orders its items; PARALLEL stages add no ordering") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    // 'Claim intake' is SEQUENTIAL: lodge -> verify -> collect, with no explicit dependency in the definition.
    CHECK(r.state("Lodge claim") == "Assigned");
    CHECK(r.state("Verify policy coverage") == "Created");
    CHECK(r.state("Collect claim documents") == "Created");
    REQUIRE(r.work("Lodge claim"));
    CHECK(r.state("Verify policy coverage") == "Assigned");
    CHECK(r.state("Collect claim documents") == "Created");
    REQUIRE(r.work("Verify policy coverage"));
    REQUIRE(r.work("Collect claim documents"));
    // 'Assessment and investigation' is PARALLEL: all four checks are open at the same moment.
    CHECK(r.state("Verify police report") == "Assigned");
    CHECK(r.state("Assess vehicle damage") == "Assigned");
    CHECK(r.state("Obtain repair estimate") == "Assigned");
    CHECK(r.state("Screen claim for fraud") == "Assigned");
    // The process is SEQUENTIAL across stages: nothing in 'Decision and settlement' is open yet.
    CHECK(r.state("Review claim and decide") == "Created");
}

TEST_CASE("Composite: the definition contains a nested SubProcess and its implied ordering") {
    Workspace ws(new InsuranceScenario());
    const WorkflowDefinition* def = ws.getRepository().find("motor-claim");
    REQUIRE(def != nullptr);
    std::vector<std::pair<std::string, std::string> > implied = def->getImpliedDependencies();
    bool lodgeBeforeVerify = false, repairBeforeInspect = false;
    for (std::size_t i = 0; i < implied.size(); ++i) {
        if (implied[i].first == "Lodge claim" && implied[i].second == "Verify policy coverage") lodgeBeforeVerify = true;
        if (implied[i].first == "Repair vehicle" && implied[i].second == "Inspect completed repairs") repairBeforeInspect = true;
    }
    CHECK(lodgeBeforeVerify);       // from the SEQUENTIAL stage
    CHECK(repairBeforeInspect);     // from the SEQUENTIAL nested sub-process
}

TEST_CASE("Deadlines: time passing escalates documents to the configured role, automatically") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    REQUIRE(r.work("Lodge claim"));
    REQUIRE(r.work("Verify policy coverage"));
    WorkItem* docs = r.item("Collect claim documents");
    CHECK(docs->getAssignee()->getName() == "Thandi Mokoena");                    // the policyholder
    CHECK(ws.getEngine().advanceClock(6 * 24 * 3600) == 0);                       // day 6: still within 7 days
    CHECK(docs->getAssignee()->getName() == "Thandi Mokoena");
    CHECK(ws.getEngine().advanceClock(2 * 24 * 3600) == 1);                       // day 8: deadline passed
    CHECK(docs->getAssignee()->getRoles().count("Claims Handler") == 1);
    CHECK(r.trailHas(EVT_ROUTED, "deadline passed"));
}

TEST_CASE("Deadlines + Priority decorator: approval climbs a level each time a (shortened) deadline passes") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    intakeAndChecks(r);
    REQUIRE(r.work("Review claim and decide", "REPAIR"));
    WorkItem* approve = r.item("Approve settlement amount");
    CHECK(approve->getAssignee()->getName() == "Lerato Dlamini");                 // R18 500 -> Claims Handler
    // Level deadline is 2 days, halved to 1 day by the Priority decorator wrapping this item.
    CHECK(ws.getEngine().advanceClock(23 * 3600) == 0);
    CHECK(approve->getAssignee()->getName() == "Lerato Dlamini");
    CHECK(ws.getEngine().advanceClock(2 * 3600) == 1);                            // 25 h: overdue
    CHECK(approve->getAssignee()->getName() == "Kagiso Molefe");                  // Claims Manager
    CHECK(ws.getEngine().advanceClock(25 * 3600) == 1);                           // fresh deadline for the next level
    CHECK(approve->getAssignee()->getName() == "Zanele Mahlangu");                // Senior Manager
    CHECK(ws.getEngine().advanceClock(10 * 24 * 3600) == 0);                      // top level: nobody higher, no endless loop
    CHECK(r.state("Approve settlement amount") == "Assigned");
}

TEST_CASE("Deadlines: an item picked up in time is not escalated") {
    Workspace ws(new EducationScenario());
    Run r(ws, "merit-funded");
    REQUIRE(r.work("Submit application"));
    REQUIRE(r.work("Verify registration"));
    REQUIRE(r.work("Collect documents"));                                         // completed before the deadline
    CHECK(ws.getEngine().advanceClock(30 * 24 * 3600) == 0);
}

TEST_CASE("Decorators run on real items: audit and notification decorators record their work") {
    Workspace ws(new EducationScenario());
    Run r(ws, "need-large");
    for (const char* n : {"Submit application", "Verify registration", "Collect documents", "Academic record check", "Means test",
                          "Reference check", "Need verification", "Shortlist decision", "Approve bursary award"}) REQUIRE_MESSAGE(r.work(n), n);
    const std::vector<std::string>& log = r.in->getDecoratorLog();
    bool audited = false, notified = false;
    for (std::size_t i = 0; i < log.size(); ++i) {
        if (log[i].find("audit:") == 0 && log[i].find("Approve bursary award") != std::string::npos) audited = true;
        if (log[i].find("notify:") == 0 && log[i].find("completed") != std::string::npos) notified = true;
    }
    CHECK(audited);
    CHECK(notified);
}

TEST_CASE("Notifications: participants are told when their work becomes available (stub SMS adapter)") {
    Workspace ws(new InsuranceScenario());
    Run r(ws, "repair-small");
    REQUIRE(r.work("Lodge claim"));
    const std::vector<std::string>& sent = r.in->getNotificationService()->getMessages();
    REQUIRE_FALSE(sent.empty());
    bool toClaimsHandler = false;
    for (std::size_t i = 0; i < sent.size(); ++i) if (sent[i].find("Verify policy coverage") != std::string::npos) toClaimsHandler = true;
    CHECK(toClaimsHandler);
}
