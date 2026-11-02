#include "ItemActions.h"
#include <map>
#include <stdexcept>

namespace {
typedef bool (*ActionFn)(WorkflowEngine&, WorkflowInstance*, WorkItem*, const std::string&);

bool doStart(WorkflowEngine& e, WorkflowInstance* i, WorkItem* w, const std::string&) { return e.startWorkItem(i, w); }
bool doComplete(WorkflowEngine& e, WorkflowInstance* i, WorkItem* w, const std::string& a) { return e.completeWorkItem(i, w, a.empty() ? "done" : a); }
bool doReject(WorkflowEngine& e, WorkflowInstance* i, WorkItem* w, const std::string& a) { return e.rejectWorkItem(i, w, a); }
bool doEscalate(WorkflowEngine& e, WorkflowInstance* i, WorkItem* w, const std::string& a) { return e.escalateWorkItem(i, w, a.empty() ? "no response in time" : a); }
bool doService(WorkflowEngine& e, WorkflowInstance* i, WorkItem* w, const std::string&) { return e.callItemService(i, w); }

struct Entry { const char* name; const char* argument; ActionFn fn; };
const Entry kActions[] = {
    { "start",    "",        &doStart },
    { "complete", "outcome", &doComplete },
    { "reject",   "reason",  &doReject },
    { "escalate", "reason",  &doEscalate },
    { "service",  "",        &doService },
};
const Entry* find(const std::string& name) {
    for (std::size_t i = 0; i < sizeof(kActions) / sizeof(kActions[0]); ++i)
        if (name == kActions[i].name) return &kActions[i];
    return 0;
}
}

std::vector<std::string> ItemActions::names() {
    std::vector<std::string> v;
    for (std::size_t i = 0; i < sizeof(kActions) / sizeof(kActions[0]); ++i) v.push_back(kActions[i].name);
    return v;
}

std::string ItemActions::argumentLabel(const std::string& action) {
    const Entry* e = find(action);
    return e ? e->argument : "";
}

bool ItemActions::run(const std::string& action, WorkflowEngine& engine, WorkflowInstance* instance,
                      WorkItem* item, const std::string& argument) {
    const Entry* e = find(action);
    if (!e) throw std::invalid_argument("unknown action: " + action);
    return e->fn(engine, instance, item, argument);
}
