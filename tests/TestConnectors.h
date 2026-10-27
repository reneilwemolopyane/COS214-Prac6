#pragma once
/**
 * @file TestConnectors.h
 * @brief Fake ExternalConnector used by the unit tests instead of real external systems.
 */
#include <string>
#include "core/Common.h"
#include "adapter/ExternalConnector.h"

/**
 * @brief Test double for an external system (e.g. a payment gateway or SMS service).
 *
 * Fails a set number of times before succeeding, so tests can check retries,
 * incidents and reversals without a real system.
 */
class FakeConnector : public ExternalConnector {
public:
    /**
     * @param failuresBeforeSuccess How many calls fail before calls start succeeding.
     * @param retryable Whether those failures are temporary (true) or business errors (false).
     */
    FakeConnector(int failuresBeforeSuccess = 0, bool retryable = true)
        : failuresLeft(failuresBeforeSuccess), retryable(retryable), calls(0), reversals(0), sends(0) {}

    std::string name() const { return "fake"; }

    /** @brief Handles "reverse", "send" and any other operation. "send" and main calls fail while failuresLeft > 0. */
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

    int failuresLeft;          ///< Remaining calls that will fail.
    bool retryable;            ///< Whether failures are temporary.
    int calls;                 ///< Main calls received.
    int reversals;             ///< "reverse" calls received.
    int sends;                 ///< "send" calls received.
    std::string lastReversed;  ///< "reference" sent with the last reversal.
    std::string lastMessage;   ///< "text" of the last message sent.
    std::string lastTo;        ///< "to" of the last message sent.
    std::string lastUrgent;    ///< "urgent" of the last message sent.
};

/**
 * @brief Test double that succeeds, returns fixed result data and remembers the last request.
 *
 * Used to check that undo builds its request from the result data
 * (e.g. "book" returns bookingRef, undo is "cancel" with bookingRef).
 */
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
    DataMap resultData;        ///< Data returned by the main call.
    ConnectorRequest last;     ///< Last request received.
};

/** @brief A generic "pay" request used across tests. */
inline ConnectorRequest payRequest() {
    ConnectorRequest req;
    req.operation = "pay";
    req.params["amount"] = "50000";
    return req;
}