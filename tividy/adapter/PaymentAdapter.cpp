#include "tividy/adapter/AdapterHelpers.h"
#include "tividy/adapter/PaymentAdapter.h"

// Checks that we can talk to the bank, then hands the request to the right method.
ConnectorResult PaymentAdapter::execute(const ConnectorRequest& request) {
    if (adaptee == 0) return failed(n, false, "no payment client");
    if (!adaptee->available()) return failed(n, true, "payment gateway unavailable");

    if (request.operation == "pay") return makePayment(request);
    if (request.operation == "reverse") return reversePayment(request);
    return failed(n, false, "unknown operation '" + request.operation + "'");
}

// "pay": checks the details, then calls PaymentAPI::submit().
ConnectorResult PaymentAdapter::makePayment(const ConnectorRequest& request) {
    long cents;
    if (!toLong(param(request.params, "amount"), cents) || cents <= 0)
        return failed(n, false, "invalid amount");

    std::string account = param(request.params, "account");
    if (account.empty()) return failed(n, false, "missing account");

    std::string txn = adaptee->submit(cents, account);
    if (txn.empty()) return failed(n, true, "payment failed, try again");

    ConnectorResult result(true, false, "paid");
    result.data["reference"] = txn;
    return result;
}

// "reverse": undoes an earlier payment, calls PaymentAPI::reverse().
ConnectorResult PaymentAdapter::reversePayment(const ConnectorRequest& request) {
    std::string reference = param(request.params, "reference");
    if (reference.empty()) return failed(n, false, "missing reference");

    if (!adaptee->reverse(reference)) return failed(n, false, "unknown or already reversed payment");
    return ConnectorResult(true, false, "reversed");
}
