#ifndef ADAPTERHELPERS_H
#define ADAPTERHELPERS_H
/**
 * @file AdapterHelpers.h
 * @brief Three small helpers used by all the adapters.
 */
#include <sstream>
#include <string>
#include "ConnectorResult.h"

/** @brief Builds a failed result whose message starts with the connector's name.
 *  @param retryable true for a temporary problem (try again), false for a permanent one. */
inline ConnectorResult failed(const std::string& connectorName, bool retryable, const std::string& message) {
    return ConnectorResult(false, retryable, connectorName + ": " + message);
}

/** @return the value stored under key, or "" if it is missing. */
inline std::string param(const ParamMap& params, const std::string& key) {
    ParamMap::const_iterator it = params.find(key);
    if (it == params.end()) return "";
    return it->second;
}

/** @brief Turns text like "5000" into a number.
 *  @return false if the text is empty, is not a whole number, or is too big. */
inline bool toLong(const std::string& text, long& number) {
    std::istringstream in(text);
    if (!(in >> number)) return false;   // not a number at all (or empty / too big)
    char leftover;
    return !(in >> leftover);            // fails only if nothing is left, e.g. not "12abc"
}
#endif
