#ifndef INSURANCESCENARIO_H
#define INSURANCESCENARIO_H
#include "Scenario.h"
#include "MotorClaimWorkflowDirector.h"
#include "tividy/adapter/RecordLookupAPI.h"
#include "tividy/adapter/PaymentAPI.h"
#include "tividy/adapter/MessageAPI.h"
#include "tividy/adapter/RecordLookupAdapter.h"
#include "tividy/adapter/PaymentAdapter.h"
#include "tividy/adapter/MessageAdapter.h"

/** @brief Industry 1: a short-term insurer handling motor-vehicle claims. */
class InsuranceScenario : public Scenario {
public:
    InsuranceScenario();
    std::vector<SampleCase> sampleCases() const override;
    std::vector<std::string> systems() const override;
    bool setOutage(const std::string& system, bool down) override;
protected:
    WorkflowDirector& director() override { return claimDirector; }
    void addParticipants(WorkflowEngine& engine) override;
    void addConnectors(ConnectorRegistry& registry) override;
private:
    MotorClaimWorkflowDirector claimDirector;
    RecordLookupAPI policyRegister, policeRegister;   // stub adaptees
    PaymentAPI bank;
    MessagingAPI sms;
    RecordLookupAdapter policyAdapter, policeAdapter; // adapters (generic classes, industry-specific names)
    PaymentAdapter paymentAdapter;
    MessagingAdapter notifyAdapter;
};
#endif
