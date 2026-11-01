#include "JsonSerializer.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/core/Participant.h"
#include "tividy/core/Common.h"
#include "tividy/observer/Observers.h"

namespace api {

json serializeComponent(WorkflowComponent* c) {
    return serializeComponent(static_cast<WorkflowInstance*>(0), c);
}

json serializeComponent(WorkflowInstance* inst, WorkflowComponent* c) {
    if (!c) return json::object();

    if (Stage* s = c->asStage()) {
        json j;
        j["type"]     = "stage";
        j["name"]     = s->getName();
        j["mode"]     = (s->getMode() == PARALLEL ? "PARALLEL" : "SEQUENTIAL");
        j["children"] = json::array();
        for (std::size_t i = 0; i < s->childCount(); ++i) {
            j["children"].push_back(serializeComponent(inst, s->childAt(i)));
        }
        return j;
    }

    if (WorkItem* w = c->underlyingItem()) {
        json j;
        j["type"]         = "workitem";
        j["id"]           = w->getId();
        j["name"]         = w->getName();
        j["role"]         = w->getRole();
        j["state"]        = w->getStateName();
        j["outcome"]      = w->getOutcome();
        j["failureReason"]= w->getFailureReason();
        j["isSkipped"]    = w->isSkipped();
        j["isComplete"]   = w->isComplete();
        j["isFinished"]   = w->isFinished();
        j["assignmentRule"] = w->getAssignmentRule();
        j["assignee"]     = w->getAssignee() ? w->getAssignee()->getName() : "";
        j["assigneeId"]   = w->getAssignee() ? w->getAssignee()->getId()   : "";

       
        std::string connector;
        if (inst) connector = inst->getItemConfig(w->getId()).serviceConnector;
        j["serviceConnector"] = connector;

        json data = json::object();
        const std::map<std::string, std::string>& dm = w->getAllData();
        for (std::map<std::string, std::string>::const_iterator it = dm.begin(); it != dm.end(); ++it) {
            data[it->first] = it->second;
        }
        j["data"] = data;
        return j;
    }

    return json::object();
}

json serializeInstance(WorkflowInstance* inst) {
    json j;
    j["id"] = inst->getId();
    j["version"] = inst->getVersion();
    j["status"] = inst->getOutcome(false);

    // Cache once — never call getAllData() twice in the same loop header.
    const DataMap& dm = inst->getAllData();
    json data = json::object();
    for (DataMap::const_iterator it = dm.begin(); it != dm.end(); ++it) {
        data[it->first] = it->second;
    }
    j["data"] = data;

    j["tree"] = serializeComponent(inst, inst->getRoot());

    j["events"] = json::array();
    if (inst->getAuditLogger()) {
        const std::vector<WorkflowEvent>& trail = inst->getAuditLogger()->getTrail();
        for (std::size_t i = 0; i < trail.size(); ++i) {
            const WorkflowEvent& ev = trail[i];
            EventType t = static_cast<EventType>(ev.getType());
            json e;
            e["type"] = static_cast<int>(t);
            e["typeName"] = eventTypeName(t);
            e["itemId"] = ev.getWorkItemId();
            e["details"] = ev.getDetails();
            e["text"] = AuditLogger::format(ev);
            j["events"].push_back(e);
        }
    }

    return j;
}

json serializeParticipant(Participant* p) {
    json j;
    j["id"] = p->getId();
    j["name"] = p->getName();
    j["workload"] = p->getWorkload();
    j["roles"] = json::array();
    for (const auto& role : p->getRoles()) j["roles"].push_back(role);
    return j;
}

}  // namespace api