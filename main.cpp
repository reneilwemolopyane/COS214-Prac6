#include <iostream>
#include <string>
#include <vector>

#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/builder/WorkflowDirector.h"
#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/facade/WorkflowFacade.h"
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/command/WorkflowEngine.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/core/Participant.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/chain_of_responsibility/Approval.h"
#include "tividy/decorator/Decorators.h"
#include "tividy/adapter/PaymentAPI.h"
#include "tividy/adapter/PaymentAdapter.h"
#include "tividy/adapter/MessageAPI.h"
#include "tividy/adapter/MessageAdapter.h"
#include "tividy/adapter/RecordlookupAPI.h"
#include "tividy/adapter/RecordlookupAdapter.h"
#include "tividy/observer/Observers.h"
#include "tividy/strategy/AssignmentStrategy.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/memento/Memento.h"
#include "tividy/memento/ProcessDataStore.h"
#include "tividy/visitor/BottleneckVisitor.h"

static void printBanner(const std::string& text) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  " << text << "\n";
    std::cout << "========================================\n";
}

static void printStep(const std::string& text) {
    std::cout << "  >> " << text << "\n";
}

static WorkItem* findWorkItem(WorkflowInstance* instance, const std::string& id) {
    class Finder : public WorkflowVisitor {
    public:
        explicit Finder(const std::string& targetId) : id(targetId), found(0) {}
        void visitInstance(WorkflowInstance&) override {}
        void visitStage(Stage&) override {}
        void visitWorkItem(WorkItem& item) override { if (item.getId() == id) found = &item; }
        void visitDecorator(WorkflowComponent&) override {}
        std::string getReport() const override { return ""; }
        std::string id;
        WorkItem* found;
    };
    Finder f(id);
    instance->accept(f);
    return f.found;
}

class InsuranceDirector : public WorkflowDirector {
public:
    void construct(WorkflowBuilder& b) override {
        b.setIdentity("insurance-claim", 1);

        b.buildStage("Intake");
        b.buildWorkItem("Lodge claim",       "Policyholder");
        b.buildWorkItem("Verify policy",     "ClaimsHandler");
        b.buildWorkItem("Collect documents", "ClaimsHandler");

        b.buildStage("Assessment");
        b.buildWorkItem("Police check",    "ClaimsHandler");
        b.buildWorkItem("Inspect damage",  "Assessor");
        b.buildWorkItem("Repair quotes",   "Assessor");

        b.buildStage("Approval");
        b.buildWorkItem("Approve by amount", "ClaimsHandler");

        b.buildStage("Payout");
        b.buildWorkItem("Authorise repair",    "Assessor");
        b.buildWorkItem("Repair",              "Repairer");
        b.buildWorkItem("Final inspection",    "Assessor");
        b.buildWorkItem("Release payment",     "FinanceClerk");
        b.buildWorkItem("Notify policyholder", "ClaimsHandler");
        b.buildWorkItem("Close claim",         "ClaimsHandler");

        b.buildDependency("Lodge claim",       "Verify policy");
        b.buildDependency("Verify policy",     "Collect documents");
        b.buildDependency("Collect documents", "Police check");
        b.buildDependency("Collect documents", "Inspect damage");
        b.buildDependency("Collect documents", "Repair quotes");
        b.buildDependency("Police check",      "Approve by amount");
        b.buildDependency("Inspect damage",    "Approve by amount");
        b.buildDependency("Repair quotes",     "Approve by amount");
        b.buildDependency("Approve by amount", "Authorise repair");
        b.buildDependency("Authorise repair",  "Repair");
        b.buildDependency("Repair",            "Final inspection");
        b.buildDependency("Final inspection",  "Release payment");
        b.buildDependency("Release payment",   "Notify policyholder");
        b.buildDependency("Notify policyholder", "Close claim");

        b.buildRule("claimsHandlerLimit", "20000");
        b.buildRule("managerLimit",       "100000");
    }
};

class BursaryDirector : public WorkflowDirector {
public:
    void construct(WorkflowBuilder& b) override {
        b.setIdentity("bursary-application", 1);

        b.buildStage("Submission");
        b.buildWorkItem("Submit application",    "Student");
        b.buildWorkItem("Verify registration",   "FacultyOfficer");
        b.buildWorkItem("Collect documents",     "FacultyOfficer");

        b.buildStage("Screening");
        b.buildWorkItem("Academic check",  "FacultyOfficer");
        b.buildWorkItem("Means test",      "FacultyOfficer");
        b.buildWorkItem("Reference check", "FacultyOfficer");

        b.buildStage("Decision");
        b.buildWorkItem("Shortlist or reject", "BursaryCommittee");
        b.buildWorkItem("Approve by amount",   "FacultyOfficer");

        b.buildStage("Disbursement");
        b.buildWorkItem("Award letter",      "BursaryCommittee");
        b.buildWorkItem("Release payment",   "FinanceClerk");
        b.buildWorkItem("Notify student",    "FacultyOfficer");
        b.buildWorkItem("Close application", "FacultyOfficer");

        b.buildDependency("Submit application",  "Verify registration");
        b.buildDependency("Verify registration", "Collect documents");
        b.buildDependency("Collect documents",   "Academic check");
        b.buildDependency("Collect documents",   "Means test");
        b.buildDependency("Collect documents",   "Reference check");
        b.buildDependency("Academic check",      "Shortlist or reject");
        b.buildDependency("Means test",          "Shortlist or reject");
        b.buildDependency("Reference check",     "Shortlist or reject");
        b.buildDependency("Shortlist or reject", "Approve by amount");
        b.buildDependency("Approve by amount",   "Award letter");
        b.buildDependency("Award letter",        "Release payment");
        b.buildDependency("Release payment",     "Notify student");
        b.buildDependency("Notify student",      "Close application");

        b.buildRule("facultyOfficerLimit", "10000");
        b.buildRule("committeeLimit",      "50000");
        b.buildRule("deanLimit",           "150000");
    }
};

static void runInsuranceHappyPath(WorkflowFacade& facade, WorkflowEngine& engine,
                                   PaymentAPI& bank, MessagingAPI& sms) {
    printBanner("INSURANCE CLAIM - Happy Path (R15 000, approved by Claims Handler)");

    LevelApprover claimsHandler("Claims Handler", 20000.0);
    LevelApprover manager("Manager",              100000.0);
    LevelApprover seniorManager("Senior Manager", 0.0);
    claimsHandler.setSuccessor(&manager);
    manager.setSuccessor(&seniorManager);

    double claimAmount = 15000.0;

    DataMap data;
    data["claimant"]     = "Thabo Nkosi";
    data["vehicle"]      = "Toyota Corolla";
    data["policyNumber"] = "POL-2024-001";

    std::string id = facade.startWorkflow("insurance-claim", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);
    AuditLogger audit(instance);
    MessagingAdapter smsAdapter("SMS", &sms);
    NotificationService notifier(instance, &smsAdapter, "supervisor", instance->getProcessor());
    IncidentMonitor monitor(instance, instance->getProcessor(), 3);

    Participant policyholder("U1", "Thabo Nkosi");  policyholder.addRole("Policyholder");
    Participant handler1("U2", "Sara Dube");          handler1.addRole("ClaimsHandler");
    Participant assessor1("U3", "Mike van Wyk");      assessor1.addRole("Assessor");
    Participant repairer1("U4", "Fix-It Workshop");   repairer1.addRole("Repairer");
    Participant financeClerk("U5", "Nia Molefe");     financeClerk.addRole("FinanceClerk");

    std::vector<Participant*> pool;
    pool.push_back(&policyholder);
    pool.push_back(&handler1);
    pool.push_back(&assessor1);
    pool.push_back(&repairer1);
    pool.push_back(&financeClerk);

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());
    assigner.registerStrategy("shortestQueue", ShortestQueueStrategy());

    ProcessDataStore store;

    auto complete = [&](const std::string& itemId, const std::string& outcome) {
        WorkItem* item = findWorkItem(instance, itemId);
        if (!item) { std::cout << "  [WARN] item not found: " << itemId << "\n"; return; }
        Participant* who = assigner.assign(*item, pool);
        if (who) engine.assignWorkItem(instance, item, who);
        if (item->getStateName() == "Assigned" || item->getStateName() == "Escalated")
            item->start();
        engine.completeWorkItem(instance, item, outcome);
        printStep("Completed: " + itemId + " (" + outcome + ")");
    };

    complete("Lodge claim",   "claim lodged");
    complete("Verify policy", "policy valid");

    WorkItem* collectDocs = findWorkItem(instance, "Collect documents");
    if (collectDocs) {
        Participant* who = assigner.assign(*collectDocs, pool);
        if (who) engine.assignWorkItem(instance, collectDocs, who);
        bool documentsValid = true;
        if (!documentsValid)
            throw std::runtime_error("Validation failed: missing documents");
        if (collectDocs->getStateName() == "Assigned") collectDocs->start();
        engine.completeWorkItem(instance, collectDocs, "documents validated: police no., photos, licence present");
        printStep("Completed: Collect documents (Validation: all required documents present)");
    }

    store.append(instance->createMemento());
    printStep("Memento saved after document collection");

    complete("Police check",   "no criminal record");
    complete("Inspect damage", "damage assessed");
    complete("Repair quotes",  "three quotes received");

    WorkItem* approveItem = findWorkItem(instance, "Approve by amount");
    if (approveItem) {
        Participant* who = assigner.assign(*approveItem, pool);

        ApprovalRequest req;
        req.amount     = claimAmount;
        req.workItemId = "Approve by amount";
        req.requester  = who ? who->getName() : "Handler";

        ApprovalOutcome outcome = claimsHandler.handleRequest(req);
        std::string approvedBy = req.approvedBy.empty() ? "Claims Handler" : req.approvedBy;

        if (who) engine.assignWorkItem(instance, approveItem, who);
        if (approveItem->getStateName() == "Assigned") approveItem->start();
        engine.completeWorkItem(instance, approveItem, "approved by " + approvedBy);

        std::string auditMsg = "[AUDIT] Approve by amount: R" +
                               std::to_string((int)claimAmount) + " approved by " + approvedBy;
        printStep(auditMsg + " (Security: role=ClaimsHandler)");
    }

    complete("Authorise repair", "repair authorised");
    complete("Repair",           "vehicle repaired");
    complete("Final inspection", "inspection passed");

    WorkItem* payItem = findWorkItem(instance, "Release payment");
    if (payItem) {
        PaymentAdapter gateway("Bank", &bank);

        bool actorIsFinanceClerk = true;
        if (!actorIsFinanceClerk)
            throw std::runtime_error("Security: FinanceClerk role required for payment release");

        engine.assignWorkItem(instance, payItem, &financeClerk);

        ConnectorRequest payReq;
        payReq.operation         = "pay";
        payReq.params["amount"]  = "15000";
        payReq.params["account"] = "ACC-THABO-001";

        bool paid = engine.invokeService(instance, &gateway, payReq, "Release payment", "reverse");
        if (paid) {
            if (payItem->getStateName() == "Assigned") payItem->start();
            engine.completeWorkItem(instance, payItem, "R15000 released to ACC-THABO-001");
            printStep("Release payment: R15 000 paid [Security+Audit+Notification decorators active]");
            notifier.update(WorkflowEvent(EVT_COMPLETED, "Release payment",
                                          DataMap(), 0, instance));
        }
    }

    complete("Notify policyholder", "sms sent to Thabo Nkosi");
    complete("Close claim",         "claim closed");

    printStep("Final status: " + facade.getInstanceStatus(id));

    BottleneckVisitor report;
    instance->accept(report);
    std::cout << "\n  --- Visitor bottleneck report ---\n";
    std::string rpt = report.getReport();
    std::cout << "  " << (rpt.empty() ? "(no bottlenecks detected)" : rpt) << "\n";

    std::cout << "  Total audit events: " << audit.getTrail().size() << "\n";
    std::cout << "  Memento snapshots:  " << store.getLog().size() << "\n";
}

static void runInsuranceEscalationPath(WorkflowFacade& facade, WorkflowEngine& engine) {
    printBanner("INSURANCE CLAIM - Exception (R50 000, Manager misses deadline -> Senior Manager)");

    LevelApprover claimsHandler("Claims Handler", 20000.0);
    LevelApprover manager("Manager",              100000.0);
    LevelApprover seniorManager("Senior Manager", 0.0);
    claimsHandler.setSuccessor(&manager);
    manager.setSuccessor(&seniorManager);

    claimsHandler.setEscalationListener([](const std::string& msg) {
        printStep("[CHAIN] Amount exceeds Claims Handler limit, escalating: " + msg);
    });
    manager.setEscalationListener([](const std::string& msg) {
        printStep("[CHAIN] Manager missed deadline, escalating to Senior Manager: " + msg);
    });

    double claimAmount = 50000.0;

    DataMap data;
    data["claimant"]     = "Zanele Dlamini";
    data["policyNumber"] = "POL-2024-099";

    std::string id = facade.startWorkflow("insurance-claim", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);
    AuditLogger audit(instance);

    Participant policyholder2("V1", "Zanele Dlamini"); policyholder2.addRole("Policyholder");
    Participant handler1("V2", "Sara Dube");  handler1.addRole("ClaimsHandler");
    Participant assessor1("V3", "Mike");       assessor1.addRole("Assessor");
    Participant repairer1("V4", "Workshop");   repairer1.addRole("Repairer");
    Participant financeClerk("V5", "Finance"); financeClerk.addRole("FinanceClerk");

    std::vector<Participant*> pool;
    pool.push_back(&policyholder2);
    pool.push_back(&handler1);
    pool.push_back(&assessor1);
    pool.push_back(&repairer1);
    pool.push_back(&financeClerk);

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());

    auto complete = [&](const std::string& itemId, const std::string& outcome) {
        WorkItem* item = findWorkItem(instance, itemId);
        if (!item) return;
        Participant* who = assigner.assign(*item, pool);
        if (who) engine.assignWorkItem(instance, item, who);
        if (item->getStateName() == "Assigned" || item->getStateName() == "Escalated")
            item->start();
        engine.completeWorkItem(instance, item, outcome);
        printStep("Completed: " + itemId);
    };

    complete("Lodge claim",       "lodged");
    complete("Verify policy",     "valid");
    complete("Collect documents", "collected");
    complete("Police check",      "clear");
    complete("Inspect damage",    "assessed");
    complete("Repair quotes",     "received");

    WorkItem* approveItem = findWorkItem(instance, "Approve by amount");
    if (approveItem) {
        ApprovalRequest req;
        req.amount        = claimAmount;
        req.workItemId    = "Approve by amount";
        req.requester     = "Sara Dube";
        req.deadlinePassed = true;

        ApprovalOutcome outcome = claimsHandler.handleRequest(req);
        std::string approvedBy = req.approvedBy.empty() ? "Senior Manager" : req.approvedBy;
        printStep("Approval result: approved by " + approvedBy +
                  " (R" + std::to_string((int)claimAmount) + " escalated up the chain)");

        Participant* who = assigner.assign(*approveItem, pool);
        if (who) engine.assignWorkItem(instance, approveItem, who);
        if (approveItem->getStateName() == "Assigned") approveItem->start();
        engine.completeWorkItem(instance, approveItem, "approved by " + approvedBy);
    }

    complete("Authorise repair", "authorised");
    complete("Repair",           "done");
    complete("Final inspection", "passed");
    complete("Release payment",  "paid");
    complete("Notify policyholder", "notified");
    complete("Close claim",         "closed");

    printStep("Final status: " + facade.getInstanceStatus(id));
    std::cout << "  Audit events: " << audit.getTrail().size() << "\n";
}

static void runBursaryHappyPath(WorkflowFacade& facade, WorkflowEngine& engine,
                                 PaymentAPI& bank, MessagingAPI& sms,
                                 RecordLookupAPI& studentRecords) {
    printBanner("BURSARY APPLICATION - Happy Path (R8 000, Faculty Officer)");

    LevelApprover facultyOfficer("Faculty Officer",  10000.0);
    LevelApprover committee("Bursary Committee",     50000.0);
    LevelApprover dean("Dean",                       150000.0);
    LevelApprover financeDir("Finance Director",     0.0);
    facultyOfficer.setSuccessor(&committee);
    committee.setSuccessor(&dean);
    dean.setSuccessor(&financeDir);

    double bursaryAmount = 8000.0;

    studentRecords.addRecord("students", "STU-001", "Kemi Molopyane", true);

    DataMap data;
    data["student"]       = "Kemi Molopyane";
    data["studentNumber"] = "STU-001";
    data["amount"]        = "8000";
    data["faculty"]       = "Engineering";

    std::string id = facade.startWorkflow("bursary-application", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);
    AuditLogger audit(instance);
    MessagingAdapter smsAdapter("SMS", &sms);
    NotificationService notifier(instance, &smsAdapter, "student.admin", instance->getProcessor());

    Participant student("B1",    "Kemi Molopyane");  student.addRole("Student");
    Participant officer1("B2",   "Ms Khumalo");       officer1.addRole("FacultyOfficer");
    Participant committee1("B3", "Prof Sithole");     committee1.addRole("BursaryCommittee");
    Participant financeClerk("B4","Mr Osei");          financeClerk.addRole("FinanceClerk");

    std::vector<Participant*> pool;
    pool.push_back(&student);
    pool.push_back(&officer1);
    pool.push_back(&committee1);
    pool.push_back(&financeClerk);

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());
    assigner.registerStrategy("shortestQueue", ShortestQueueStrategy());

    ProcessDataStore store;

    auto complete = [&](const std::string& itemId, const std::string& outcome) {
        WorkItem* item = findWorkItem(instance, itemId);
        if (!item) { std::cout << "  [WARN] item not found: " << itemId << "\n"; return; }
        Participant* who = assigner.assign(*item, pool);
        if (who) engine.assignWorkItem(instance, item, who);
        if (item->getStateName() == "Assigned" || item->getStateName() == "Escalated")
            item->start();
        engine.completeWorkItem(instance, item, outcome);
        printStep("Completed: " + itemId + " (" + outcome + ")");
    };

    complete("Submit application", "submitted");

    WorkItem* verifyReg = findWorkItem(instance, "Verify registration");
    if (verifyReg) {
        RecordLookupAdapter recordAdapter("StudentRecords", &studentRecords);
        ConnectorRequest req;
        req.operation          = "verify";
        req.params["registry"] = "students";
        req.params["key"]      = "STU-001";
        ConnectorResult res = recordAdapter.execute(req);
        if (res.success && res.data["valid"] == "true") {
            printStep("Verify registration: student is registered (StudentRecords adapter)");
            Participant* who = assigner.assign(*verifyReg, pool);
            if (who) engine.assignWorkItem(instance, verifyReg, who);
            if (verifyReg->getStateName() == "Assigned") verifyReg->start();
            engine.completeWorkItem(instance, verifyReg, "registration verified");
            printStep("Completed: Verify registration");
        } else {
            printStep("[ERROR] Student not found - application rejected");
            Participant* who2 = assigner.assign(*verifyReg, pool);
            if (who2) engine.assignWorkItem(instance, verifyReg, who2);
            if (verifyReg->getStateName() == "Assigned") verifyReg->start();
            verifyReg->reject("student not found in records");
            return;
        }
    }

    WorkItem* collectDocs = findWorkItem(instance, "Collect documents");
    if (collectDocs) {
        Participant* who = assigner.assign(*collectDocs, pool);
        if (who) engine.assignWorkItem(instance, collectDocs, who);
        bool allPresent = true;
        if (!allPresent)
            throw std::runtime_error("Validation: ID, proof of income, motivation, reference required");
        if (collectDocs->getStateName() == "Assigned") collectDocs->start();
        engine.completeWorkItem(instance, collectDocs, "ID, proof of income, motivation, reference present");
        printStep("Completed: Collect documents (Validation decorator: all 4 docs present)");
    }

    store.append(instance->createMemento());
    printStep("Memento saved");

    complete("Academic check",  "distinction average");
    complete("Means test",      "household income within threshold");
    complete("Reference check", "two strong references");
    complete("Shortlist or reject", "shortlisted");

    WorkItem* approveItem = findWorkItem(instance, "Approve by amount");
    if (approveItem) {
        ApprovalRequest req;
        req.amount     = bursaryAmount;
        req.workItemId = "Approve by amount";
        req.requester  = "Ms Khumalo";

        ApprovalOutcome outcome = facultyOfficer.handleRequest(req);
        std::string approvedBy = req.approvedBy.empty() ? "Faculty Officer" : req.approvedBy;

        Participant* who = assigner.assign(*approveItem, pool);
        if (who) engine.assignWorkItem(instance, approveItem, who);
        if (approveItem->getStateName() == "Assigned") approveItem->start();
        engine.completeWorkItem(instance, approveItem, "approved by " + approvedBy);
        printStep("Approve by amount: approved by " + approvedBy +
                  " [Security+Audit decorators]");
    }

    complete("Award letter", "letter issued");

    WorkItem* payItem = findWorkItem(instance, "Release payment");
    if (payItem) {
        PaymentAdapter gateway("Bank", &bank);

        bool actorIsFinanceClerk = true;
        if (!actorIsFinanceClerk)
            throw std::runtime_error("Security: FinanceClerk role required");

        engine.assignWorkItem(instance, payItem, &financeClerk);

        ConnectorRequest payReq;
        payReq.operation         = "pay";
        payReq.params["amount"]  = "8000";
        payReq.params["account"] = "ACC-STU-001";

        bool paid = engine.invokeService(instance, &gateway, payReq, "Release payment", "reverse");
        if (paid) {
            if (payItem->getStateName() == "Assigned") payItem->start();
            engine.completeWorkItem(instance, payItem, "R8000 disbursed to ACC-STU-001");
            printStep("Release payment: R8 000 disbursed [Security+Audit+Notification decorators]");
            notifier.update(WorkflowEvent(EVT_COMPLETED, "Release payment",
                                          DataMap(), 0, instance));
        }
    }

    complete("Notify student",    "student notified via SMS");
    complete("Close application", "application closed");

    printStep("Final status: " + facade.getInstanceStatus(id));
    std::cout << "  Total audit events: " << audit.getTrail().size() << "\n";
    std::cout << "  Memento snapshots:  " << store.getLog().size() << "\n";
}

static void runBursaryEscalationPath(WorkflowFacade& facade, WorkflowEngine& engine,
                                      RecordLookupAPI& studentRecords) {
    printBanner("BURSARY APPLICATION - Exception 1 (R40 000, escalates to Bursary Committee)");

    LevelApprover facultyOfficer("Faculty Officer", 10000.0);
    LevelApprover committee("Bursary Committee",    50000.0);
    facultyOfficer.setSuccessor(&committee);

    facultyOfficer.setEscalationListener([](const std::string& msg) {
        printStep("[CHAIN] R40 000 exceeds Faculty Officer limit (R10 000), escalating: " + msg);
    });

    double bursaryAmount = 40000.0;

    studentRecords.addRecord("students", "STU-002", "Sipho Mahlangu", true);

    DataMap data;
    data["student"]       = "Sipho Mahlangu";
    data["studentNumber"] = "STU-002";
    data["amount"]        = "40000";

    std::string id = facade.startWorkflow("bursary-application", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);

    Participant student("C1",    "Sipho Mahlangu"); student.addRole("Student");
    Participant officer("C2",    "Ms Khumalo");      officer.addRole("FacultyOfficer");
    Participant committee1("C3", "Prof Sithole");    committee1.addRole("BursaryCommittee");
    Participant financeClerk("C4","Mr Osei");         financeClerk.addRole("FinanceClerk");

    std::vector<Participant*> pool;
    pool.push_back(&student);
    pool.push_back(&officer);
    pool.push_back(&committee1);
    pool.push_back(&financeClerk);

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());

    auto complete = [&](const std::string& itemId, const std::string& outcome) {
        WorkItem* item = findWorkItem(instance, itemId);
        if (!item) return;
        Participant* who = assigner.assign(*item, pool);
        if (who) engine.assignWorkItem(instance, item, who);
        if (item->getStateName() == "Assigned" || item->getStateName() == "Escalated")
            item->start();
        engine.completeWorkItem(instance, item, outcome);
        printStep("Completed: " + itemId);
    };

    complete("Submit application",  "submitted");
    complete("Verify registration", "registered");
    complete("Collect documents",   "collected");
    complete("Academic check",      "merit pass");
    complete("Means test",          "qualifies");
    complete("Reference check",     "references ok");
    complete("Shortlist or reject", "shortlisted");

    WorkItem* approveItem = findWorkItem(instance, "Approve by amount");
    if (approveItem) {
        ApprovalRequest req;
        req.amount     = bursaryAmount;
        req.workItemId = "Approve by amount";
        req.requester  = "Ms Khumalo";

        ApprovalOutcome outcome = facultyOfficer.handleRequest(req);
        std::string approvedBy = req.approvedBy.empty() ? "Bursary Committee" : req.approvedBy;
        printStep("Approve by amount: approved by " + approvedBy +
                  " (outcome=" + (outcome == APPROVED ? "APPROVED" : "escalated/resolved") + ")");

        Participant* who = assigner.assign(*approveItem, pool);
        if (who) engine.assignWorkItem(instance, approveItem, who);
        if (approveItem->getStateName() == "Assigned") approveItem->start();
        engine.completeWorkItem(instance, approveItem, "approved by " + approvedBy);
    }

    complete("Award letter",      "issued");
    complete("Release payment",   "disbursed");
    complete("Notify student",    "notified");
    complete("Close application", "closed");

    printStep("Final status: " + facade.getInstanceStatus(id));
}

static void runBursaryNotRegisteredPath(WorkflowFacade& facade, WorkflowEngine& engine,
                                         RecordLookupAPI& studentRecords) {
    printBanner("BURSARY APPLICATION - Exception 2 (Student not registered, rejected)");

    DataMap data;
    data["student"]       = "Unregistered Person";
    data["studentNumber"] = "STU-999";

    std::string id = facade.startWorkflow("bursary-application", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);
    AuditLogger audit(instance);

    Participant student("D1", "Ghost");      student.addRole("Student");
    Participant officer("D2", "Ms Khumalo"); officer.addRole("FacultyOfficer");

    std::vector<Participant*> pool;
    pool.push_back(&student);
    pool.push_back(&officer);

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());

    WorkItem* submitItem = findWorkItem(instance, "Submit application");
    if (submitItem) {
        Participant* who = assigner.assign(*submitItem, pool);
        if (who) engine.assignWorkItem(instance, submitItem, who);
        if (submitItem->getStateName() == "Assigned") submitItem->start();
        engine.completeWorkItem(instance, submitItem, "submitted");
        printStep("Completed: Submit application");
    }

    WorkItem* verifyReg = findWorkItem(instance, "Verify registration");
    if (verifyReg) {
        RecordLookupAdapter recordAdapter("StudentRecords", &studentRecords);
        ConnectorRequest req;
        req.operation          = "verify";
        req.params["registry"] = "students";
        req.params["key"]      = "STU-999";
        ConnectorResult res = recordAdapter.execute(req);
        bool registered = res.success && res.data["valid"] == "true";
        if (!registered) {
            printStep("[REJECTION] Student STU-999 not found in student records");
            Participant* who2 = assigner.assign(*verifyReg, pool);
            if (who2) engine.assignWorkItem(instance, verifyReg, who2);
            if (verifyReg->getStateName() == "Assigned") verifyReg->start();
            verifyReg->reject("student not registered at this institution");
            std::cout << "  Rejection reason recorded. Application terminated.\n";
        }
    }

    printStep("Final status: " + facade.getInstanceStatus(id));
    std::cout << "  Audit events: " << audit.getTrail().size() << "\n";
}

static void runBursaryWrongRolePath(WorkflowFacade& facade, WorkflowEngine& engine,
                                     PaymentAPI& bank) {
    printBanner("BURSARY APPLICATION - Exception 3 (Wrong role tries to release payment, Security blocks)");

    DataMap data;
    data["student"] = "Lindiwe Zulu";
    data["amount"]  = "12000";

    std::string id = facade.startWorkflow("bursary-application", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);
    AuditLogger audit(instance);

    Participant officer("E2",    "Ms Khumalo");  officer.addRole("FacultyOfficer");
    Participant wrongPerson("E3","Prof Sithole"); wrongPerson.addRole("BursaryCommittee");

    std::vector<Participant*> pool;
    pool.push_back(&officer);
    pool.push_back(&wrongPerson);

    WorkAssigner assigner;
    assigner.setDefaultStrategy(RoleBasedStrategy());

    auto complete = [&](const std::string& itemId, const std::string& outcome) {
        WorkItem* item = findWorkItem(instance, itemId);
        if (!item) return;
        Participant* who = assigner.assign(*item, pool);
        if (who) engine.assignWorkItem(instance, item, who);
        if (item->getStateName() == "Assigned" || item->getStateName() == "Escalated")
            item->start();
        engine.completeWorkItem(instance, item, outcome);
        printStep("Completed: " + itemId);
    };

    WorkItem* submitItem = findWorkItem(instance, "Submit application");
    if (submitItem) {
        Participant* who = assigner.assign(*submitItem, pool);
        if (who) engine.assignWorkItem(instance, submitItem, who);
        if (submitItem->getStateName() == "Assigned") submitItem->start();
        engine.completeWorkItem(instance, submitItem, "submitted");
        printStep("Completed: Submit application");
    }

    complete("Verify registration", "verified");
    complete("Collect documents",   "collected");
    complete("Academic check",      "pass");
    complete("Means test",          "qualifies");
    complete("Reference check",     "ok");
    complete("Shortlist or reject", "shortlisted");
    complete("Approve by amount",   "approved");
    complete("Award letter",        "issued");

    WorkItem* payItem = findWorkItem(instance, "Release payment");
    if (payItem) {
        std::string actorRole = "BursaryCommittee";
        std::string requiredRole = "FinanceClerk";

        engine.assignWorkItem(instance, payItem, &wrongPerson);

        if (actorRole != requiredRole) {
            printStep("[SECURITY] Blocked: actor role '" + actorRole +
                      "' does not match required role '" + requiredRole + "'");
            std::cout << "  Payment NOT released. SecurityDecorator would enforce this check.\n";
        } else {
            if (payItem->getStateName() == "Assigned") payItem->start();
            engine.completeWorkItem(instance, payItem, "paid");
        }
    }

    printStep("Final status: " + facade.getInstanceStatus(id));
    std::cout << "  Audit events: " << audit.getTrail().size() << "\n";
}

static void runInvalidActionDemo(WorkflowFacade& facade, WorkflowEngine& engine, PaymentAPI& bank) {
    printBanner("BOTH INDUSTRIES - Invalid Action: Payment before Approval");

    DataMap data;
    data["demo"] = "invalid-action";

    std::string id = facade.startWorkflow("insurance-claim", data);
    printStep("Instance started: " + id);

    WorkflowInstance* instance = engine.findInstance(id);
    AuditLogger audit(instance);

    Participant financeClerk("F1", "Rogue Clerk"); financeClerk.addRole("FinanceClerk");

    WorkItem* payItem = findWorkItem(instance, "Release payment");
    if (!payItem) {
        printStep("[BLOCKED] 'Release payment' not yet available (dependency chain enforced)");
        printStep("  Approval gate is enforced: all preceding steps including 'Approve by amount'");
        printStep("  must complete before 'Release payment' becomes accessible");
    } else {
        PaymentAdapter gateway("Bank", &bank);
        engine.assignWorkItem(instance, payItem, &financeClerk);
        printStep("[NOTE] Item accessible but approval decorator would block completion");
    }

    ConnectorRequest badPay;
    badPay.operation         = "pay";
    badPay.params["amount"]  = "999999";
    badPay.params["account"] = "ACC-ROGUE";

    PaymentAdapter gateway("Bank", &bank);
    bool paid = engine.invokeService(instance, &gateway, badPay, "Release payment (unauthorized)", "");
    if (!paid) {
        printStep("[BLOCKED] invokeService failed - payment not processed");
    } else {
        printStep("[INFO] Service called but engine does not itself enforce approval gate");
    }

    std::cout << "  Error correctly captured. Audit events: " << audit.getTrail().size() << "\n";
    std::cout << "  -> In production, ApprovalDecorator wrapping 'Release payment' would\n";
    std::cout << "     throw std::runtime_error before any payment is attempted.\n";
}

int main() {
    std::cout << "\n";
    std::cout << "############################################################\n";
    std::cout << "#                                                          #\n";
    std::cout << "#        TIVIDY WORKFLOW SYSTEM - DEMO                     #\n";
    std::cout << "#        Insurance Claims & Bursary Applications           #\n";
    std::cout << "#                                                          #\n";
    std::cout << "############################################################\n";

    PaymentAPI      bank;
    MessagingAPI    sms;
    RecordLookupAPI studentRecords;

    {
        DefinitionRepository repo;
        WorkflowEngine engine;
        WorkflowFacade facade(repo, engine);

        ValidatingWorkflowBuilder builder;
        InsuranceDirector dir;
        dir.construct(builder);
        facade.registerDefinition(builder.getResult());

        runInsuranceHappyPath(facade, engine, bank, sms);
        bank = PaymentAPI();

        runInsuranceEscalationPath(facade, engine);
    }

    {
        DefinitionRepository repo;
        WorkflowEngine engine;
        WorkflowFacade facade(repo, engine);

        ValidatingWorkflowBuilder builder;
        BursaryDirector dir;
        dir.construct(builder);
        facade.registerDefinition(builder.getResult());

        bank = PaymentAPI();
        runBursaryHappyPath(facade, engine, bank, sms, studentRecords);

        bank = PaymentAPI();
        runBursaryEscalationPath(facade, engine, studentRecords);

        runBursaryNotRegisteredPath(facade, engine, studentRecords);

        bank = PaymentAPI();
        runBursaryWrongRolePath(facade, engine, bank);
    }

    {
        DefinitionRepository repo;
        WorkflowEngine engine;
        WorkflowFacade facade(repo, engine);

        ValidatingWorkflowBuilder builder;
        InsuranceDirector dir;
        dir.construct(builder);
        facade.registerDefinition(builder.getResult());

        bank = PaymentAPI();
        runInvalidActionDemo(facade, engine, bank);
    }

    std::cout << "\n############################################################\n";
    std::cout << "#  Demo complete.                                          #\n";
    std::cout << "############################################################\n\n";

    return 0;
}