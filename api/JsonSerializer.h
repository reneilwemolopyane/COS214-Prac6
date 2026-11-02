#pragma once
#include "json.hpp"
#include <string>

class WorkflowComponent;
class WorkflowInstance;
class Participant;

namespace api {
    using json = nlohmann::json;

    /** Recursively serialises a WorkflowComponent (Stage or WorkItem) to JSON. */
    json serializeComponent(WorkflowComponent* c);
    json serializeComponent(WorkflowInstance* inst, WorkflowComponent* c);

    /** Serialises the entire WorkflowInstance, including its tree, events, and data. */
    json serializeInstance(WorkflowInstance* inst);

    /** Serialises a Participant. */
    json serializeParticipant(Participant* p);
}