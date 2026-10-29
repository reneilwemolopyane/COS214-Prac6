#pragma once
#include <string>
#include "tividy/core/Common.h"
#include "tividy/adapter/ExternalConnector.h"

class FakeConnector : public ExternalConnector {
public:
        FakeConnector(int failuresBeforeSuccess = 0, bool retryable = true)
        : failuresLeft(failuresBeforeSuccess), retryable(retryable), calls(0), reversals(0), sends(0) {}

    std::string name() const { return "fake"; }

        ConnectorResult execute(const ConnectorRequest& request) {
        ConnectorResult r;
        if (request.operation == "reverse") {
            ++reversals;
            DataMap::const_iterator ref = request.params.find("reference");
            lastReversed = ref != request.params.end() ? ref->second : "";
            r.success = true;
            return r;
        }
        if (request.operation == "send") {
            if (failuresLeft > 0) {
                --failuresLeft;
                r.retryable = retryable;
                r.message = "messaging unavailable";
                return r;
            }
            ++sends;
            DataMap::const_iterator msg = request.params.find("text");
            lastMessage = msg != request.params.end() ? msg->second : "";
            DataMap::const_iterator to = request.params.find("to");
            lastTo = to != request.params.end() ? to->second : "";
            DataMap::const_iterator urgent = request.params.find("urgent");
            lastUrgent = urgent != request.params.end() ? urgent->second : "";
            r.success = true;
            return r;
        }
        ++calls;
        if (failuresLeft > 0) {
            --failuresLeft;
            r.retryable = retryable;
            r.message = retryable ? "service unavailable" : "rejected by service";
            return r;
        }
        r.success = true;
        r.data["reference"] = "REF-001";
        return r;
    }

    int failuresLeft;
    bool retryable;
    int calls;
    int reversals;
    int sends;
    std::string lastReversed;
    std::string lastMessage;
    std::string lastTo;
    std::string lastUrgent;
};

class RecordingConnector : public ExternalConnector {
public:
    explicit RecordingConnector(const DataMap& resultData) : resultData(resultData) {}
    std::string name() const { return "recording"; }
    ConnectorResult execute(const ConnectorRequest& request) {
        last = request;
        ConnectorResult r;
        r.success = true;
        if (request.operation != "cancel" && request.operation != "delete" && request.operation != "reverse")
            r.data = resultData;
        return r;
    }
    DataMap resultData;
    ConnectorRequest last;
};

inline ConnectorRequest payRequest() {
    ConnectorRequest req;
    req.operation = "pay";
    req.params["amount"] = "50000";
    return req;
}
