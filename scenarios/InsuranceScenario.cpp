#include "InsuranceScenario.h"
#include "tividy/core/Participant.h"

InsuranceScenario::InsuranceScenario()
    : Scenario("motor-claim", "Insurance", "Motor vehicle accident claim",
               "A short-term insurer takes a claim from the first report to repair or write-off payout: "
               "policy check, parallel police/damage/quote/fraud checks, approval by amount, repair, final "
               "inspection, payment and closure."),
      policyAdapter("policy-admin", &policyRegister),
      policeAdapter("police-system", &policeRegister),
      paymentAdapter("payment", &bank),
      notifyAdapter("notify", &sms) {
    policyRegister.addRecord("policies", "POL-1001", "comprehensive", true);
    policyRegister.addRecord("policies", "POL-2002", "comprehensive (lapsed)", false);
    policeRegister.addRecord("cases", "CAS-114/10/2026", "accident report filed", true);
}

void InsuranceScenario::addParticipants(WorkflowEngine& engine) {
    struct Person { const char* id; const char* name; const char* role; };
    static const Person people[] = {
        {"p1", "Thandi Mokoena", "Policyholder"},   {"p2", "Lerato Dlamini", "Claims Handler"},
        {"p3", "Mpho Khumalo", "Claims Handler"},   {"p4", "Pieter van Wyk", "Assessor"},
        {"p5", "Sipho Nkosi", "Panel Beater"},      {"p6", "Nomsa Zulu", "Fraud Investigator"},
        {"p7", "Kagiso Molefe", "Claims Manager"},  {"p8", "Zanele Mahlangu", "Senior Manager"},
        {"p9", "Ayanda Cele", "Finance Clerk"}};
    for (std::size_t i = 0; i < sizeof(people) / sizeof(people[0]); ++i) {
        Participant* p = new Participant(people[i].id, people[i].name);
        p->addRole(people[i].role);
        engine.addParticipant(p);
    }
}

void InsuranceScenario::addConnectors(ConnectorRegistry& registry) {
    registry.add("policy-admin", &policyAdapter);
    registry.add("police-system", &policeAdapter);
    registry.add("payment", &paymentAdapter);
    registry.add("notify", &notifyAdapter);
}

std::vector<std::string> InsuranceScenario::systems() const {
    std::vector<std::string> s;
    s.push_back("payment"); s.push_back("policy-admin"); s.push_back("police-system"); s.push_back("notify");
    return s;
}

bool InsuranceScenario::setOutage(const std::string& system, bool down) {
    if (system == "payment") bank.setDown(down);
    else if (system == "policy-admin") policyRegister.setDown(down);
    else if (system == "police-system") policeRegister.setDown(down);
    else if (system == "notify") sms.setDown(down);
    else return false;
    return true;
}

std::vector<SampleCase> InsuranceScenario::sampleCases() const {
    std::vector<SampleCase> out;
    SampleCase base;
    base.data["policyNumber"] = "POL-1001"; base.data["claimant"] = "Thandi Mokoena";
    base.data["policeCaseNumber"] = "CAS-114/10/2026"; base.data["bankAccount"] = "62012345678";
    base.data["accidentDescription"] = "Rear-ended at a traffic light"; base.data["fraudScore"] = "12";

    SampleCase a = base; a.key = "repair-small"; a.label = "Small claim, repair (R18 500)";
    a.story = "Happy path. Handler-level approval, repair route, payment after the final inspection.";
    a.data["claimAmount"] = "18500"; out.push_back(a);

    SampleCase b = base; b.key = "writeoff-large"; b.label = "Write-off, large claim (R245 000)";
    b.story = "Review decides WRITE_OFF: the repair branch is skipped, a cash payout is authorised. Amount needs the Senior Manager.";
    b.data["claimAmount"] = "245000"; out.push_back(b);

    SampleCase c = base; c.key = "suspicious"; c.label = "Suspicious claim (fraud score 85)";
    c.story = "Fraud score routes the claim to the fraud investigator. Reject at review, or let it continue, then cancel after payment to reverse it.";
    c.data["claimAmount"] = "64000"; c.data["fraudScore"] = "85"; out.push_back(c);

    SampleCase d = base; d.key = "lapsed-policy"; d.label = "Lapsed policy";
    d.story = "Start 'Verify policy coverage', call its external system (the policy system answers valid=false), then reject it to see the rejection cascade.";
    d.data["policyNumber"] = "POL-2002"; d.data["claimAmount"] = "30000"; out.push_back(d);

    SampleCase e = base; e.key = "missing-docs"; e.label = "Documents missing (no police case number)";
    e.story = "The validation decorator refuses to complete 'Collect claim documents'. Escalate it (7 days, no response) or supply the data.";
    e.data["claimAmount"] = "52000"; e.data.erase("policeCaseNumber"); out.push_back(e);
    return out;
}
