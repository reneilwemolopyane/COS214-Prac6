#include "AdapterHelpers.h"
#include "RecordLookupAdapter.h"


// Checks the request, then that the service is reachable, then hands it to the right method.
ConnectorResult RecordLookupAdapter::execute(const ConnectorRequest& request) {
    if (adaptee == 0) return failed(n, false, "no lookup client");

    const std::string& op = request.operation;
    if (op != "verify" && op != "lookup") return failed(n, false, "unknown operation '" + op + "'");

    std::string registry = param(request.params, "registry");
    std::string key = param(request.params, "key");
    if (registry.empty() || key.empty()) return failed(n, false, "missing registry or key");

    if (!adaptee->available()) return failed(n, true, "lookup service unavailable");

    if (op == "verify") return verify(registry, key);
    return lookup(registry, key);
}

// "verify": a record that exists and is active is valid. Anything else is a normal "not valid" answer.
ConnectorResult RecordLookupAdapter::verify(const std::string& registry, const std::string& key) {
    std::string value; bool active = false;
    bool found = adaptee->find(registry, key, value, active);
    bool valid = found && active;

    ConnectorResult result(true, false, valid ? "valid" : "not valid");
    result.data["valid"] = valid ? "true" : "false";
    if (found) result.data["value"] = value;
    return result;
}

// "lookup": returns the stored value, or a permanent failure if there is no such record.
ConnectorResult RecordLookupAdapter::lookup(const std::string& registry, const std::string& key) {
    std::string value; bool active = false;
    if (!adaptee->find(registry, key, value, active)) return failed(n, false, "record not found");

    ConnectorResult result(true, false, "found");
    result.data["value"] = value;
    result.data["active"] = active ? "true" : "false";
    return result;
}