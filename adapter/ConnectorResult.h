#ifndef CONNECTORRESULT_H
#define CONNECTORRESULT_H
/**
 * @file ConnectorResult.h
 * @brief The two simple types every adapter uses: a request going out and a result coming back.
 *
 * No industry-specific words here. What an operation means is decided by the scenario
 * that sets up the connector.
 */
#include <string>
#include <map>

/** @brief A list of name/value pairs, e.g. params["amount"] = "5000". */
typedef std::map<std::string, std::string> ParamMap;

/** @brief What we ask an external system to do. */
struct ConnectorRequest {
    std::string operation;   ///< What to do, e.g. "pay", "send", "book".
    ParamMap params;         ///< The details, e.g. amount, account.
};

/** @brief The answer, in the same shape no matter which system replied. */
struct ConnectorResult {
    bool success;            ///< true if it worked.
    bool retryable;          ///< true if it failed for a temporary reason (e.g. service down).
    std::string message;     ///< Short explanation, used in the history.
    ParamMap data;           ///< Values sent back, e.g. data["reference"].

    ConnectorResult() : success(false), retryable(false) {}
    ConnectorResult(bool ok, bool retry, const std::string& msg)
        : success(ok), retryable(retry), message(msg) {}
};
#endif
