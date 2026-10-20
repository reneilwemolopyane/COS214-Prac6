#pragma once
/** @file ConnectorResult.h  Generic request/response types. No industry terms here. (ADAPTER OWNER) */
#include <string>
#include "tividy/core/Common.h"
/** @brief Generic request: operation name + parameters. Meaning is defined by the scenario in main.cpp. */
struct ConnectorRequest {
    std::string operation;
    DataMap params;
};
/** @brief Common result every adapter converts its adaptee's response into. */
struct ConnectorResult {
    bool success;      ///< call succeeded
    bool retryable;    ///< temporary failure -> retry, then raise incident
    std::string message;
    DataMap data;
    ConnectorResult() : success(false), retryable(false) {}
};
