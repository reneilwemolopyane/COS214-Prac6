#pragma once
#include "tividy/core/Common.h"
/** @brief Event data carried from subject to observers (Observer). STUB */
struct WorkflowEvent {
    EventType type;
    std::string instanceId, workItemId, timestamp, details, commandName;
    WorkflowEvent() : type(EVT_CREATED) {}
};
