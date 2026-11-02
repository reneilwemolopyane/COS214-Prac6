#ifndef EDUCATIONSCENARIO_H
#define EDUCATIONSCENARIO_H
#include "Scenario.h"
#include "EducationBursaryWorkflowDirector.h"
#include "tividy/adapter/RecordLookupAPI.h"
#include "tividy/adapter/PaymentAPI.h"
#include "tividy/adapter/MessageAPI.h"
#include "tividy/adapter/RecordLookupAdapter.h"
#include "tividy/adapter/PaymentAdapter.h"
#include "tividy/adapter/MessageAdapter.h"

/** @brief Industry 2: a university financial-aid office awarding funded bursaries. */
class EducationScenario : public Scenario {
public:
    EducationScenario();
    std::vector<SampleCase> sampleCases() const override;
    std::vector<std::string> systems() const override;
    bool setOutage(const std::string& system, bool down) override;
protected:
    WorkflowDirector& director() override { return bursaryDirector; }
    void addParticipants(WorkflowEngine& engine) override;
    void addConnectors(ConnectorRegistry& registry) override;
private:
    EducationBursaryWorkflowDirector bursaryDirector;
    RecordLookupAPI studentRecords, incomeService;    // stub adaptees
    PaymentAPI bank;
    MessagingAPI sms;
    RecordLookupAdapter studentAdapter, incomeAdapter;
    PaymentAdapter paymentAdapter;
    MessagingAdapter notifyAdapter;
};
#endif
