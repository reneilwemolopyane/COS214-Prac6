#pragma once
/** @file Common.h  Shared enums and typedefs used by every TIVIDY module. (STUB) */
#include <string>
#include <map>
#include <vector>

typedef std::map<std::string, std::string> DataMap;

enum EventType { EVT_CREATED, EVT_ASSIGNED, EVT_STARTED, EVT_COMPLETED,
                 EVT_FAILED, EVT_REJECTED, EVT_CANCELLED, EVT_ESCALATED,
                 EVT_COMPENSATED, EVT_TIMED_OUT, EVT_SERVICE_CALLED,
                 EVT_SKIPPED, EVT_ROUTED, EVT_REFUSED };
/** @brief Readable name of an event type, for the audit trail and the UI. */
inline const char* eventTypeName(EventType t) {
    switch (t) {
        case EVT_CREATED:     return "CREATED";
        case EVT_ASSIGNED:    return "ASSIGNED";
        case EVT_STARTED:     return "STARTED";
        case EVT_COMPLETED:   return "COMPLETED";
        case EVT_FAILED:      return "FAILED";
        case EVT_REJECTED:    return "REJECTED";
        case EVT_CANCELLED:   return "CANCELLED";
        case EVT_ESCALATED:   return "ESCALATED";
        case EVT_COMPENSATED: return "COMPENSATED";
        case EVT_TIMED_OUT:   return "TIMED_OUT";
        case EVT_SERVICE_CALLED: return "SERVICE_CALLED";
        case EVT_SKIPPED:     return "SKIPPED";
        case EVT_ROUTED:      return "ROUTED";
        case EVT_REFUSED:     return "REFUSED";
    }
    return "UNKNOWN";
}
enum ExecutionMode { SEQUENTIAL, PARALLEL };
enum ApprovalOutcome { APPROVED, REJECTED, ESCALATED, MORE_INFO };
