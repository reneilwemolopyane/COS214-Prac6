#pragma once
/** @file Common.h  Shared enums and typedefs used by every TIVIDY module. (STUB) */
#include <string>
#include <map>
#include <vector>

typedef std::map<std::string, std::string> DataMap;

enum EventType { EVT_CREATED, EVT_ASSIGNED, EVT_STARTED, EVT_COMPLETED,
                 EVT_FAILED, EVT_REJECTED, EVT_CANCELLED, EVT_ESCALATED };
enum ExecutionMode { SEQUENTIAL, PARALLEL };
enum ApprovalOutcome { APPROVED, REJECTED, ESCALATED, MORE_INFO };
