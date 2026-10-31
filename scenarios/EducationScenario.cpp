#include "EducationScenario.h"
#include "tividy/core/Participant.h"

EducationScenario::EducationScenario()
    : Scenario("bursary-application", "Education", "University bursary application",
               "A university financial-aid office takes a bursary application from submission to payment: "
               "registration check, parallel academic/means/reference checks, merit or need route, "
               "fund check (waitlist), approval by amount, award acceptance and payment."),
      studentAdapter("student-records", &studentRecords),
      incomeAdapter("income-verification", &incomeService),
      paymentAdapter("payment", &bank),
      notifyAdapter("notify", &sms) {
    studentRecords.addRecord("students", "u25123456", "BSc Computer Science", true);
    studentRecords.addRecord("students", "u25999999", "deregistered", false);
    studentRecords.addRecord("results", "u25123456", "76", true);
    studentRecords.addRecord("results", "u25777777", "58", true);
    studentRecords.addRecord("students", "u25777777", "BCom Accounting", true);
    incomeService.addRecord("income", "u25123456", "low", true);
    incomeService.addRecord("income", "u25777777", "low", true);
}

void EducationScenario::addParticipants(WorkflowEngine& engine) {
    struct Person { const char* id; const char* name; const char* role; };
    static const Person people[] = {
        {"e1", "Naledi Sithole", "Student"},          {"e2", "Busisiwe Ndlovu", "Bursary Officer"},
        {"e3", "Johan Pretorius", "Registrar's Office"}, {"e4", "Fatima Patel", "Financial Aid Assessor"},
        {"e5", "Prof. Mokoena", "Faculty Officer"},   {"e6", "Dr Khumalo", "Bursary Committee"},
        {"e7", "Dean Naidoo", "Dean"},                {"e8", "Ms Smit", "Finance Director"},
        {"e9", "Thabo Mabasa", "Finance Clerk"},      {"e10", "Karabo Moeketsi", "Bursary Officer"}};
    for (std::size_t i = 0; i < sizeof(people) / sizeof(people[0]); ++i) {
        Participant* p = new Participant(people[i].id, people[i].name);
        p->addRole(people[i].role);
        engine.addParticipant(p);
    }
}

void EducationScenario::addConnectors(ConnectorRegistry& registry) {
    registry.add("student-records", &studentAdapter);
    registry.add("income-verification", &incomeAdapter);
    registry.add("payment", &paymentAdapter);
    registry.add("notify", &notifyAdapter);
}

std::vector<std::string> EducationScenario::systems() const {
    std::vector<std::string> s;
    s.push_back("payment"); s.push_back("student-records"); s.push_back("income-verification"); s.push_back("notify");
    return s;
}

bool EducationScenario::setOutage(const std::string& system, bool down) {
    if (system == "payment") bank.setDown(down);
    else if (system == "student-records") studentRecords.setDown(down);
    else if (system == "income-verification") incomeService.setDown(down);
    else if (system == "notify") sms.setDown(down);
    else return false;
    return true;
}

std::vector<SampleCase> EducationScenario::sampleCases() const {
    std::vector<SampleCase> out;
    SampleCase base;
    base.data["studentNumber"] = "u25123456"; base.data["academicAverage"] = "76";
    base.data["householdIncome"] = "180000"; base.data["bursaryType"] = "merit";
    base.data["fundBalance"] = "900000"; base.data["bankAccount"] = "10099887766";
    base.data["idDocument"] = "id-copy.pdf"; base.data["motivationLetter"] = "letter.pdf";

    SampleCase a = base; a.key = "merit-funded"; a.label = "Merit bursary, R8 000";
    a.story = "Merit route (need route skipped). Faculty Officer approves, student accepts, payment is released.";
    a.data["requestedAmount"] = "8000"; out.push_back(a);

    SampleCase b = base; b.key = "need-large"; b.label = "Need bursary, R120 000";
    b.story = "Need route (merit route skipped). The amount climbs the chain to the Dean.";
    b.data["bursaryType"] = "need"; b.data["requestedAmount"] = "120000"; out.push_back(b);

    SampleCase c = base; c.key = "fund-exhausted"; c.label = "Fund exhausted (waitlist)";
    c.story = "Fund balance is below the request: the award branch is skipped and the student is waitlisted and notified.";
    c.data["requestedAmount"] = "60000"; c.data["fundBalance"] = "20000"; out.push_back(c);

    SampleCase d = base; d.key = "deregistered"; d.label = "Student not registered";
    d.story = "Start 'Verify registration', call its external system (student records answer valid=false), then reject it to see the rejection cascade.";
    d.data["studentNumber"] = "u25999999"; d.data["requestedAmount"] = "15000"; out.push_back(d);

    SampleCase e = base; e.key = "missing-docs"; e.label = "ID document missing";
    e.story = "The validation decorator refuses 'Collect documents'. Escalate to the Bursary Officer or supply the document.";
    e.data["requestedAmount"] = "9000"; e.data.erase("idDocument"); out.push_back(e);
    return out;
}
