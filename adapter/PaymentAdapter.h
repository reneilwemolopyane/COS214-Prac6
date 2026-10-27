#ifndef PAYMENTADAPTER_H
#define PAYMENTADAPTER_H
#include <string>
#include "ExternalConnector.h"
#include "PaymentAPI.h"

/** @brief Generic ADAPTER for any payment/bank service (any industry).
 *  Operations (request.operation):
 *   "pay"      params: amount (cents, > 0), account      -> data["reference"]
 *   "reverse"  params: reference                         (compensation; works once per payment)
 *  Retryable failure = the gateway is down. Everything else is a permanent failure.
 *  The adapter does not own the PaymentAPI it wraps. */
class PaymentAdapter : public ExternalConnector {
public:
    PaymentAdapter(const std::string& name, PaymentAPI* api) : n(name), adaptee(api) {}
    std::string name() const { return n; }
    /** @brief Translates a generic request into PaymentAPI calls and the reply into a ConnectorResult. */
    ConnectorResult execute(const ConnectorRequest& request);
private:
    ConnectorResult makePayment(const ConnectorRequest& request);
    ConnectorResult reversePayment(const ConnectorRequest& request);

    std::string n; PaymentAPI* adaptee;
};

#endif
