#include "IntegrationFixture.h"
#include "tividy/adapter/MessageAPI.h"
#include "tividy/adapter/MessageAdapter.h"
#include "tividy/adapter/PaymentAPI.h"
#include "tividy/adapter/PaymentAdapter.h"
#include "tividy/command/CommandProcessor.h"

namespace {

WorkflowInstance* startClaim(IntegrationFixture& f) {
    WorkflowBuilder& b = f.define("motor-claim");
    b.buildStage("Claim");
    b.buildWorkItem("Lodge claim", "Policyholder");
    b.buildWorkItem("Assess damage", "Assessor");
    b.buildWorkItem("Pay out", "FinanceClerk");
    return f.start();
}

ConnectorRequest payment(const std::string& amountCents) {
    ConnectorRequest r;
    r.operation = "pay";
    r.params["amount"] = amountCents;
    r.params["account"] = "ACC-1042";
    return r;
}

}

TEST_SUITE("Kemi - Command, Observer, Adapter") {

TEST_CASE("Command: engine commands drive a real WorkItem through its states") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    Participant* thandi = f.person("P1", "Thandi", "Policyholder");
    WorkItem* lodge = f.item("Lodge claim");
    REQUIRE(lodge != 0);
    CHECK(lodge->getStateName() == "Available");

    CHECK(f.engine.assignWorkItem(claim, lodge, thandi));
    CHECK(lodge->getStateName() == "Assigned");
    CHECK(lodge->getAssignee() == thandi);
    CHECK(thandi->getWorkload() == 1);

    lodge->start();
    CHECK(f.engine.completeWorkItem(claim, lodge, "accident reported"));
    CHECK(lodge->getStateName() == "Completed");
    CHECK(lodge->getOutcome() == "accident reported");
    CHECK(thandi->getWorkload() == 0);
    CHECK(claim->getProcessor()->getHistory().size() == 2);
}

TEST_CASE("Command: an illegal move is refused and leaves the WorkItem unchanged") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    WorkItem* assess = f.item("Assess damage");
    REQUIRE(assess != 0);

    CHECK_FALSE(f.engine.completeWorkItem(claim, assess, "too early"));
    CHECK(assess->getStateName() == "Available");
    CHECK(assess->getOutcome().empty());

    SUBCASE("the failure is announced with the State's reason") {
        REQUIRE(f.count(EVT_FAILED, "Assess damage", "CompleteWorkItem") == 1);
        const WorkflowEvent& failed = f.audit->getTrail()[f.indexOf(EVT_FAILED, "Assess damage")];
        CHECK(failed.get("message").find("Cannot complete") != std::string::npos);
    }
}

TEST_CASE("Command: cancelling an instance undoes completed work, newest first") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    Participant* thandi = f.person("P1", "Thandi", "Policyholder");
    WorkItem* lodge = f.item("Lodge claim");
    REQUIRE(f.engine.assignWorkItem(claim, lodge, thandi));
    lodge->start();
    REQUIRE(f.engine.completeWorkItem(claim, lodge, "reported"));

    CHECK(f.engine.cancel(claim->getId()));
    CHECK(f.engine.getStatus(claim->getId()) == "CANCELLED");
    CHECK(lodge->getStateName() == "InProgress");
    CHECK(lodge->getOutcome().empty());
    CHECK(f.count(EVT_COMPENSATED, "Lodge claim") == 2);
    CHECK(f.indexOf(EVT_COMPENSATED, "Lodge claim") > f.indexOf(EVT_COMPLETED, "Lodge claim"));
}

TEST_CASE("Observer: every action on the instance reaches the audit trail in order") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    Participant* thandi = f.person("P1", "Thandi", "Policyholder");
    WorkItem* lodge = f.item("Lodge claim");
    f.engine.assignWorkItem(claim, lodge, thandi);
    lodge->start();
    f.engine.completeWorkItem(claim, lodge, "reported");

    int assigned = f.indexOf(EVT_ASSIGNED, "Lodge claim");
    int started = f.indexOf(EVT_STARTED, "Lodge claim");
    int completed = f.indexOf(EVT_COMPLETED, "Lodge claim");
    CHECK(assigned >= 0);
    CHECK(started > assigned);
    CHECK(completed > started);
    CHECK(f.count(EVT_ASSIGNED, "Lodge claim", "AssignWorkItem") == 1);
    CHECK(f.count(EVT_COMPLETED, "Lodge claim", "CompleteWorkItem") == 1);
    const std::vector<WorkflowEvent>& trail = f.audit->getTrail();
    for (std::size_t i = 0; i < trail.size(); ++i)
        if (trail[i].get("command") == "CompleteWorkItem") CHECK(trail[i].getSource() == claim);
}

TEST_CASE("Observer: IncidentMonitor raises exactly one incident for a refused action") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    IncidentMonitor monitor(claim, claim->getProcessor(), 3);

    f.engine.completeWorkItem(claim, f.item("Pay out"), "paid");

    REQUIRE(monitor.getIncidents().size() == 1);
    CHECK(monitor.getIncidents()[0].find("Pay out") != std::string::npos);
    CHECK(monitor.getIncidents()[0].find("not retryable") != std::string::npos);
    CHECK(f.count(EVT_FAILED, "Pay out") == 1);
}

TEST_CASE("Observer: each instance's observers only hear their own instance") {
    IntegrationFixture f;
    WorkflowInstance* first = startClaim(f);
    WorkflowInstance* second = f.startAnother();
    REQUIRE(second != first);
    Participant* thandi = f.person("P1", "Thandi", "Policyholder");

    f.engine.assignWorkItem(first, f.item("Lodge claim", first), thandi);

    CHECK(f.count(EVT_ASSIGNED, "Lodge claim") == 1);
    const std::vector<WorkflowEvent>& other = f.trail(second);
    for (std::size_t i = 0; i < other.size(); ++i) CHECK(other[i].getType() != EVT_ASSIGNED);
    CHECK(f.item("Lodge claim", second)->getStateName() == "Available");
}

TEST_CASE("Adapter: SMS outage is retried as a command, then becomes one incident") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    MessagingAPI sms;
    MessagingAdapter channel("SMS", &sms);
    NotificationService notifications(claim, &channel, "supervisor", claim->getProcessor());
    IncidentMonitor monitor(claim, claim->getProcessor(), 3);
    Participant* thandi = f.person("P1", "Thandi", "Policyholder");

    sms.setDown(true);
    CHECK(f.engine.assignWorkItem(claim, f.item("Lodge claim"), thandi));

    CHECK(f.item("Lodge claim")->getStateName() == "Assigned");
    CHECK(f.count(EVT_FAILED, "Lodge claim", "SendNotification") == 3);
    REQUIRE(monitor.getIncidents().size() == 1);
    CHECK(monitor.getIncidents()[0].find("retries exhausted") != std::string::npos);

    SUBCASE("once the service is back, the next notification is sent") {
        sms.setDown(false);
        Participant* lerato = f.person("P2", "Lerato", "Assessor");
        CHECK(f.engine.assignWorkItem(claim, f.item("Assess damage"), lerato));
        CHECK(f.count(EVT_NOTIFIED, "Assess damage", "SendNotification") == 1);
        CHECK(monitor.getIncidents().size() == 1);
    }
}

TEST_CASE("Adapter: a payment made through the engine is reversed when the claim is cancelled") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    PaymentAPI bank;
    PaymentAdapter gateway("Bank", &bank);

    CHECK(f.engine.invokeService(claim, &gateway, payment("4500000"), "Pay out", "reverse"));
    REQUIRE(bank.knows("TXN-1"));

    CHECK(f.engine.cancel(claim->getId()));
    CHECK(f.count(EVT_COMPENSATED, "Pay out") >= 1);
    CHECK_FALSE(bank.reverse("TXN-1"));
}

TEST_CASE("Adapter: a temporary bank outage is retried until the payment goes through") {
    IntegrationFixture f;
    WorkflowInstance* claim = startClaim(f);
    PaymentAPI bank;
    PaymentAdapter gateway("Bank", &bank);
    IncidentMonitor monitor(claim, claim->getProcessor(), 3);

    bank.setDown(true);
    CHECK_FALSE(f.engine.invokeService(claim, &gateway, payment("4500000"), "Pay out", "reverse"));
    CHECK(f.count(EVT_FAILED, "Pay out", "InvokeService") == 3);
    CHECK(monitor.getIncidents().size() == 1);
    CHECK_FALSE(bank.knows("TXN-1"));

    SUBCASE("a business error from the adapter is not retried") {
        bank.setDown(false);
        CHECK_FALSE(f.engine.invokeService(claim, &gateway, payment("-5"), "Pay out", "reverse"));
        CHECK(f.count(EVT_FAILED, "Pay out", "InvokeService") == 4);
        CHECK(monitor.getIncidents().size() == 2);
    }
}

}