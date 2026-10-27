#pragma once
/** @file Common.h  Shared enums and typedefs used by every TIVIDY module. */
#include <string>
#include <map>
#include <vector>

/** @brief Generic key/value data (instance data, request parameters). */
typedef std::map<std::string, std::string> DataMap;

/** @brief Kinds of workflow events pushed to observers and sent to the Mediator. */
enum EventType { EVT_CREATED, EVT_ASSIGNED, EVT_STARTED, EVT_COMPLETED, EVT_FAILED, EVT_REJECTED,
                 EVT_CANCELLED, EVT_ESCALATED, EVT_TIMED_OUT, EVT_COMPENSATED, EVT_NOTIFIED };
enum ExecutionMode { SEQUENTIAL, PARALLEL };
enum ApprovalOutcome { APPROVED, REJECTED, ESCALATED, MORE_INFO };

/** @brief Readable name of an event type, e.g. "FAILED". */
inline std::string eventName(EventType t) {
    switch (t) {
        case EVT_CREATED:     return "CREATED";
        case EVT_ASSIGNED:    return "ASSIGNED";
        case EVT_STARTED:     return "STARTED";
        case EVT_COMPLETED:   return "COMPLETED";
        case EVT_FAILED:      return "FAILED";
        case EVT_REJECTED:    return "REJECTED";
        case EVT_CANCELLED:   return "CANCELLED";
        case EVT_ESCALATED:   return "ESCALATED";
        case EVT_TIMED_OUT:   return "TIMED_OUT";
        case EVT_COMPENSATED: return "COMPENSATED";
        case EVT_NOTIFIED:    return "NOTIFIED";
    }
    return "UNKNOWN";
}