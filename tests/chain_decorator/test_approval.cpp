#include "doctest/doctest.h"
#include <stdexcept>
#include <string>
#include <vector>
#include "Approval.h"

namespace {

ApprovalRequest makeReq(double amount) {
    ApprovalRequest r;
    r.amount = amount;
    r.workItemId = "REQ-1";
    return r;
}

} // namespace

TEST_CASE("approves when the amount is within the level's limit") {
    LevelApprover handler("Handler", 20000);
    ApprovalRequest r = makeReq(15000);

    CHECK(handler.handleRequest(r) == APPROVED);
    CHECK(r.approvedBy == "Handler");
}

TEST_CASE("no decision function means the level approves by default") {
    LevelApprover handler("Handler", 100);
    ApprovalRequest r = makeReq(100);   // exactly at the limit is still allowed

    CHECK(handler.handleRequest(r) == APPROVED);
}

TEST_CASE("escalates to the next level when the amount is over the limit") {
    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000);
    handler.setSuccessor(&manager);

    ApprovalRequest r = makeReq(45000);

    CHECK(handler.handleRequest(r) == APPROVED);
    CHECK(r.approvedBy == "Manager");
}

TEST_CASE("walks through three levels") {
    LevelApprover a("Handler", 20000);
    LevelApprover b("Manager", 100000);
    LevelApprover c("Senior Manager", 1000000);
    a.setSuccessor(&b)->setSuccessor(&c);

    ApprovalRequest r = makeReq(500000);

    CHECK(a.handleRequest(r) == APPROVED);
    CHECK(r.approvedBy == "Senior Manager");
}

TEST_CASE("a level that decides ESCALATED passes the request up even within its limit") {
    int managerAsked = 0;
    LevelApprover handler("Handler", 20000,
        [](ApprovalRequest&) { return ESCALATED; });
    LevelApprover manager("Manager", 100000,
        [&managerAsked](ApprovalRequest&) { ++managerAsked; return APPROVED; });
    handler.setSuccessor(&manager);

    ApprovalRequest r = makeReq(5000);

    CHECK(handler.handleRequest(r) == APPROVED);
    CHECK(managerAsked == 1);
    CHECK(r.approvedBy == "Manager");
}

TEST_CASE("a rejection ends the chain and keeps the reason") {
    int seniorAsked = 0;
    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000,
        [](ApprovalRequest& req) { req.reason = "Policy lapsed"; return REJECTED; });
    LevelApprover senior("Senior Manager", 1000000,
        [&seniorAsked](ApprovalRequest&) { ++seniorAsked; return APPROVED; });
    handler.setSuccessor(&manager)->setSuccessor(&senior);

    ApprovalRequest r = makeReq(50000);

    CHECK(handler.handleRequest(r) == REJECTED);
    CHECK(r.reason == "Policy lapsed");
    CHECK(r.approvedBy.empty());
    CHECK(seniorAsked == 0);   // chain stopped
}

TEST_CASE("a rejection at the first level never reaches the next one") {
    int managerAsked = 0;
    LevelApprover handler("Handler", 20000,
        [](ApprovalRequest& req) { req.reason = "Missing documents"; return REJECTED; });
    LevelApprover manager("Manager", 100000,
        [&managerAsked](ApprovalRequest&) { ++managerAsked; return APPROVED; });
    handler.setSuccessor(&manager);

    ApprovalRequest r = makeReq(1000);

    CHECK(handler.handleRequest(r) == REJECTED);
    CHECK(managerAsked == 0);
}

TEST_CASE("returns ESCALATED when no level in the chain has enough authority") {
    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000);
    handler.setSuccessor(&manager);

    ApprovalRequest r = makeReq(9999999);

    CHECK(handler.handleRequest(r) == ESCALATED);   // engine applies the default decision
    CHECK(r.approvedBy.empty());
}

TEST_CASE("a passed deadline makes a level skip itself without being asked") {
    int handlerAsked = 0;
    LevelApprover handler("Handler", 20000,
        [&handlerAsked](ApprovalRequest&) { ++handlerAsked; return APPROVED; });
    LevelApprover manager("Manager", 100000);
    handler.setSuccessor(&manager);

    ApprovalRequest r = makeReq(100);
    r.deadlinePassed = true;

    CHECK(handler.handleRequest(r) == APPROVED);
    CHECK(handlerAsked == 0);
    CHECK(r.approvedBy == "Manager");
    CHECK_FALSE(r.deadlinePassed);   // next level got a fresh deadline
}

TEST_CASE("a passed deadline at the end of the chain returns ESCALATED") {
    LevelApprover only("Only", 100000);
    ApprovalRequest r = makeReq(100);
    r.deadlinePassed = true;

    CHECK(only.handleRequest(r) == ESCALATED);
}

TEST_CASE("setSuccessor refuses to create a loop") {
    LevelApprover a("A", 1), b("B", 2), c("C", 3);
    a.setSuccessor(&b)->setSuccessor(&c);

    CHECK_THROWS_AS(c.setSuccessor(&a), std::invalid_argument);   // A -> B -> C -> A
    CHECK_THROWS_AS(a.setSuccessor(&a), std::invalid_argument);   // itself
}

TEST_CASE("setSuccessor returns the successor so calls can be chained") {
    LevelApprover a("A", 1), b("B", 2);
    CHECK(a.setSuccessor(&b) == &b);
}

TEST_CASE("escalation listener is told about each escalation") {
    std::vector<std::string> heard;
    ApprovalHandler::EscalationListener listener =
        [&heard](const std::string& s) { heard.push_back(s); };

    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000);
    LevelApprover senior("Senior Manager", 1000000);
    handler.setSuccessor(&manager)->setSuccessor(&senior);
    handler.setEscalationListener(listener);
    manager.setEscalationListener(listener);

    ApprovalRequest r = makeReq(500000);
    handler.handleRequest(r);

    REQUIRE(heard.size() == 2);
    CHECK(heard[0] == "Handler escalated REQ-1 to Manager");
    CHECK(heard[1] == "Manager escalated REQ-1 to Senior Manager");
}

TEST_CASE("no escalation message when the first level decides") {
    std::vector<std::string> heard;
    LevelApprover handler("Handler", 20000);
    LevelApprover manager("Manager", 100000);
    handler.setSuccessor(&manager);
    handler.setEscalationListener(
        [&heard](const std::string& s) { heard.push_back(s); });

    ApprovalRequest r = makeReq(100);
    handler.handleRequest(r);

    CHECK(heard.empty());
}

TEST_CASE("requests sent through the same chain do not affect each other") {
    LevelApprover handler("Handler", 20000,
        [](ApprovalRequest& req) {
            if (req.workItemId == "BAD") { req.reason = "Fraud"; return REJECTED; }
            return APPROVED;
        });

    ApprovalRequest bad = makeReq(100);
    bad.workItemId = "BAD";
    ApprovalRequest good = makeReq(100);
    good.workItemId = "GOOD";

    CHECK(handler.handleRequest(bad) == REJECTED);
    CHECK(handler.handleRequest(good) == APPROVED);
    CHECK(bad.approvedBy.empty());
    CHECK(bad.reason == "Fraud");
    CHECK(good.approvedBy == "Handler");
    CHECK(good.reason.empty());
}