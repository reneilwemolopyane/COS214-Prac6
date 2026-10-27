#pragma once
/** @file Common.h  Shared enums and typedefs used by every TIVIDY module. */
#include <string>
#include <map>
#include <vector>

/** @brief Generic key/value data (instance data, request parameters). */
typedef std::map<std::string, std::string> DataMap;

/** @brief Kinds of workflow events pushed to observers and sent to the Mediator. */
enum class EventType { CREATED, ASSIGNED, STARTED, COMPLETED, FAILED, REJECTED,
                       CANCELLED, ESCALATED, TIMED_OUT, COMPENSATED, NOTIFIED };
enum ExecutionMode { SEQUENTIAL, PARALLEL };
enum ApprovalOutcome { APPROVED, REJECTED, ESCALATED, MORE_INFO };

/** @brief Readable name of an event type, e.g. "FAILED". */
inline std::string eventName(EventType t) {
    switch (t) {
        case EventType::CREATED:     return "CREATED";
        case EventType::ASSIGNED:    return "ASSIGNED";
        case EventType::STARTED:     return "STARTED";
        case EventType::COMPLETED:   return "COMPLETED";
        case EventType::FAILED:      return "FAILED";
        case EventType::REJECTED:    return "REJECTED";
        case EventType::CANCELLED:   return "CANCELLED";
        case EventType::ESCALATED:   return "ESCALATED";
        case EventType::TIMED_OUT:   return "TIMED_OUT";
        case EventType::COMPENSATED: return "COMPENSATED";
        case EventType::NOTIFIED:    return "NOTIFIED";
    }
    return "UNKNOWN";
}