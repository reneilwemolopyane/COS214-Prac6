#include "ScenarioCatalog.h"
#include "InsuranceScenario.h"
#include "EducationScenario.h"
#include <map>

namespace {
// One factory function per industry. Adding an industry means adding one line here; no branching on ids.
typedef Scenario* (*Factory)();
Scenario* makeInsurance() { return new InsuranceScenario(); }
Scenario* makeEducation() { return new EducationScenario(); }

struct Entry { const char* id; Factory make; };
const Entry kEntries[] = { { "motor-claim", &makeInsurance }, { "bursary-application", &makeEducation } };
}

std::vector<std::string> ScenarioCatalog::ids() {
    std::vector<std::string> v;
    for (std::size_t i = 0; i < sizeof(kEntries) / sizeof(kEntries[0]); ++i) v.push_back(kEntries[i].id);
    return v;
}

Scenario* ScenarioCatalog::create(const std::string& id) {
    for (std::size_t i = 0; i < sizeof(kEntries) / sizeof(kEntries[0]); ++i)
        if (id == kEntries[i].id) return kEntries[i].make();
    return 0;
}
