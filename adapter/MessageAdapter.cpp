#include "AdapterHelpers.h"
#include "MessageAdapter.h"

// Only one operation, "send": checks the details, then calls MessagingAPI::send().
ConnectorResult MessagingAdapter::execute(const ConnectorRequest& request) {
    if (adaptee == 0) return failed(n, false, "no messaging client");
    if (!adaptee->available()) return failed(n, true, "messaging service unavailable");
    if (request.operation != "send") return failed(n, false, "unknown operation '" + request.operation + "'");

    std::string to = param(request.params, "to");
    if (to.empty()) return failed(n, false, "missing recipient");

    std::string text = param(request.params, "text");
    if (text.empty()) return failed(n, false, "empty message");

    bool urgent = (param(request.params, "urgent") == "true");
    if (!adaptee->send(to, text, urgent)) return failed(n, true, "send failed, try again");
    return ConnectorResult(true, false, "sent");
}
