#include "ApiServer.h"
#include "JsonSerializer.h"
#include <iostream>
#include <stdexcept>

#include "tividy/core/Participant.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Common.h"
#include "tividy/observer/Observers.h"
#include "tividy/command/Command.h"
#include "tividy/memento/ProcessDataStore.h"
#include "tividy/visitor/AuditReportVisitor.h"
#include "tividy/visitor/BottleneckVisitor.h"

namespace api {

namespace {
struct Located {
    WorkflowInstance* inst;
    WorkflowEngine*   engine;
    Workspace*        ws;
    std::string       scenarioId;
    Located() : inst(0), engine(0), ws(0) {}
    bool ok() const { return inst != 0; }
};

Located locate(Session& session, const std::string& instId) {
    Located l;
    const std::vector<std::string>& ids = session.ids();
    for (std::size_t i = 0; i < ids.size(); ++i) {
        Workspace* ws = session.find(ids[i]);
        if (!ws) continue;
        WorkflowInstance* inst = ws->getEngine().findInstance(instId);
        if (inst) {
            l.inst       = inst;
            l.engine     = &ws->getEngine();
            l.ws         = ws;
            l.scenarioId = ids[i];
            return l;
        }
    }
    return l;
}
}  // namespace

ApiServer::ApiServer(Session& session, const std::string& webRoot, int port)
    : session(session), port(port) {
    if (!svr.set_mount_point("/", webRoot.c_str())) {
        std::cerr << "[WARN] Web root not found: " << webRoot << "\n";
    }
}

void ApiServer::setupRoutes() {
    // ==================== Definitions ====================
    svr.Get("/api/definitions", [this](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        const std::vector<std::string>& ids = session.ids();
        for (std::size_t i = 0; i < ids.size(); ++i) {
            Workspace* ws = session.find(ids[i]);
            if (!ws) continue;
            Scenario& sc = ws->getScenario();
            json j;
            j["id"]       = sc.id();
            j["industry"] = sc.industry();
            j["title"]    = sc.title();
            j["summary"]  = sc.summary();
            arr.push_back(j);
        }
        res.set_content(json{{"ok", true}, {"definitions", arr}}.dump(),
                        "application/json");
    });

    // Sample cases for a scenario
    svr.Get(R"(/api/definitions/([^/]+)/cases)",
            [this](const httplib::Request& req, httplib::Response& res) {
        Workspace* ws = session.find(req.matches[1].str());
        if (!ws) { res.status = 404; return; }
        json arr = json::array();
        std::vector<SampleCase> cases = ws->getScenario().sampleCases();
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const SampleCase& sc = cases[i];
            json j;
            j["key"]   = sc.key;
            j["label"] = sc.label;
            j["story"] = sc.story;
            json d = json::object();
            for (DataMap::const_iterator it = sc.data.begin(); it != sc.data.end(); ++it)
                d[it->first] = it->second;
            j["data"] = d;
            arr.push_back(j);
        }
        res.set_content(json{{"ok", true}, {"cases", arr}}.dump(), "application/json");
    });

    // ==================== Participants (scoped to a scenario) ====================
    svr.Get("/api/participants", [this](const httplib::Request& req, httplib::Response& res) {
        std::string sid = req.has_param("scenarioId")
                            ? req.get_param_value("scenarioId")
                            : session.ids()[0];
        Workspace* ws = session.find(sid);
        if (!ws) {
            res.status = 404;
            res.set_content(json{{"ok", false}, {"error", "Unknown scenario: " + sid}}.dump(),
                            "application/json");
            return;
        }
        json arr = json::array();
        const std::vector<Participant*>& pool = ws->getEngine().getParticipants();
        for (std::size_t i = 0; i < pool.size(); ++i) arr.push_back(serializeParticipant(pool[i]));
        res.set_content(json{{"ok", true}, {"participants", arr}}.dump(), "application/json");
    });

    // ==================== Instances list ====================
    svr.Get("/api/instances", [this](const httplib::Request&, httplib::Response& res) {
        json arr = json::array();
        const std::vector<std::string>& ids = session.ids();
        for (std::size_t i = 0; i < ids.size(); ++i) {
            Workspace* ws = session.find(ids[i]);
            if (!ws) continue;
            const std::vector<WorkflowInstance*>& insts = ws->getEngine().getInstances();
            for (std::size_t k = 0; k < insts.size(); ++k) {
                WorkflowInstance* inst = insts[k];
                json j;
                j["id"]         = inst->getId();
                j["scenarioId"] = ids[i];
                j["status"]     = ws->getEngine().getStatus(inst->getId());
                j["outcome"]    = inst->getOutcome(ws->getEngine().getStatus(inst->getId()) == "CANCELLED");
                arr.push_back(j);
            }
        }
        res.set_content(json{{"ok", true}, {"instances", arr}}.dump(), "application/json");
    });

    // ==================== Start workflow ====================
    svr.Post("/api/instances", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            auto body = json::parse(req.body);
            std::string scenarioId = body["scenarioId"];
            Workspace* ws = session.find(scenarioId);
            if (!ws) throw std::runtime_error("Invalid scenario ID");

            DataMap dm;
            if (body.contains("data") && body["data"].is_object()) {
                for (auto it = body["data"].begin(); it != body["data"].end(); ++it) {
                    if (it.value().is_string())
                        dm[it.key()] = it.value().get<std::string>();
                    else
                        dm[it.key()] = it.value().dump();
                }
            }

            std::string instId = ws->getFacade().startWorkflow(scenarioId, dm);
            res.status = 201;
            res.set_content(json{{"ok", true},
                                 {"id", instId},
                                 {"scenarioId", scenarioId}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                            "application/json");
        }
    });

    // ==================== Get instance detail ====================
    svr.Get(R"(/api/instances/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }
        json instJson = serializeInstance(l.inst);
        instJson["status"] = l.inst->getOutcome(l.engine->getStatus(l.inst->getId()) == "CANCELLED");
        res.set_content(json{
            {"ok", true},
            {"instance", instJson},
            {"scenarioId", l.scenarioId}
        }.dump(), "application/json");
    });

    // ==================== Cancel instance ====================
    svr.Post(R"(/api/instances/([^/]+)/cancel)", [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }
        try {
            if (l.engine->cancel(l.inst->getId())) {
                res.set_content(json{{"ok", true}}.dump(), "application/json");
            } else {
                res.status = 400;
                res.set_content(json{{"ok", false},
                                     {"error", "Cancel failed (instance not running)"}}.dump(),
                                "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                            "application/json");
        }
    });

    // ==================== Undo last step (Memento rollback) ====================
    svr.Post(R"(/api/instances/([^/]+)/undo)", [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }
        InstanceMemento* m = session.storeFor(l.inst->getId()).undo();
        if (!m) {
            res.set_content(json{{"ok", true}, {"undone", false}}.dump(),
                            "application/json");
            return;
        }
        try {
            l.inst->restoreFromMemento(*m);
            delete m;
            l.inst->raiseEvent(EVT_ROUTED, "", 0,
                "rolled back to the previous checkpoint (history is kept)");
            res.set_content(json{{"ok", true}, {"undone", true}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            delete m;
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                            "application/json");
        }
    });

    // ==================== Side effects (Observers + Command history) ====================
    svr.Get(R"(/api/instances/([^/]+)/side-effects)",
            [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }

        json notifications = json::array();
        const std::vector<std::string>& msgs = l.inst->getNotificationService()->getMessages();
        for (std::size_t i = 0; i < msgs.size(); ++i) notifications.push_back(msgs[i]);

        json incidents = json::array();
        const std::vector<std::string>& inc = l.inst->getIncidentMonitor()->getIncidents();
        for (std::size_t i = 0; i < inc.size(); ++i) incidents.push_back(inc[i]);

        json decorators = json::array();
        const std::vector<std::string>& dec = l.inst->getDecoratorLog();
        for (std::size_t i = 0; i < dec.size(); ++i) decorators.push_back(dec[i]);

        json commands = json::array();
        const std::vector<WorkflowCommand*>& hist = l.inst->getProcessor()->getHistory();
        for (std::size_t i = 0; i < hist.size(); ++i) {
            WorkflowCommand* c = hist[i];
            json cj;
            cj["name"]     = c->name();
            cj["itemId"]   = c->getWorkItemId();
            cj["status"]   = c->getStatus();
            cj["attempts"] = c->getAttempts();
            cj["error"]    = c->getError();
            commands.push_back(cj);
        }

        res.set_content(json{
            {"ok", true},
            {"notifications", notifications},
            {"incidents", incidents},
            {"decorators", decorators},
            {"commands", commands}
        }.dump(), "application/json");
    });

    // ==================== Reports (Visitor) ====================
    svr.Get(R"(/api/instances/([^/]+)/reports)",
            [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }
        AuditReportVisitor audit;
        l.inst->accept(audit);
        BottleneckVisitor slow;
        l.inst->accept(slow);
        res.set_content(json{
            {"ok", true},
            {"audit", audit.getReport()},
            {"bottlenecks", slow.getReport()}
        }.dump(), "application/json");
    });

    // ==================== Work item actions (P0 + checkpoint + setActor) ====================
    svr.Post(R"(/api/instances/([^/]+)/items/([^/]+)/([\w-]+))",
             [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }

        const std::string itemId = req.matches[2].str();
        const std::string action = req.matches[3].str();

        WorkItem* item = l.inst->getCoordinator()->findItem(itemId);
        if (!item) { res.status = 404; return; }

        // Validate action name
        static const char* const kKnown[] =
            {"start","complete","reject","escalate","call-service","assign"};
        bool known = false;
        for (std::size_t i = 0; i < sizeof(kKnown)/sizeof(kKnown[0]); ++i)
            if (action == kKnown[i]) { known = true; break; }
        if (!known) {
            res.status = 400;
            res.set_content(json{{"ok", false},
                                 {"error", "Unknown action: " + action}}.dump(),
                            "application/json");
            return;
        }

        // call-service only makes sense on items with an external system
        if (action == "call-service") {
            ItemConfig cfg = l.inst->getItemConfig(item->getId());
            if (cfg.serviceConnector.empty()) {
                res.status = 400;
                res.set_content(json{{"ok", false},
                                     {"error", "This item has no external system configured"}}.dump(),
                                "application/json");
                return;
            }
        }

        // Parse body once
        json body = json::object();
        if (!req.body.empty()) {
            try { body = json::parse(req.body); }
            catch (const std::exception& e) {
                res.status = 400;
                res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                                "application/json");
                return;
            }
        }

        // Resolve actor (mirrors TUI's actAs): assignee by default, body.actor overrides
        Participant* actor = item->getAssignee();
        std::string aid = body.value("actor", "");
        if (!aid.empty()) {
            actor = 0;
            const std::vector<Participant*>& pool = l.engine->getParticipants();
            for (std::size_t i = 0; i < pool.size(); ++i)
                if (pool[i]->getId() == aid) { actor = pool[i]; break; }
            if (!actor) {
                res.status = 400;
                res.set_content(json{{"ok", false},
                                     {"error", "Unknown actor: " + aid}}.dump(),
                                "application/json");
                return;
            }
        }

        // Memento checkpoint before any state-changing action (matches TUI)
        const bool checkpointed = (action != "call-service");
        ProcessDataStore& store = session.storeFor(l.inst->getId());
        if (checkpointed) store.append(l.inst->createMemento());

        // Act
        l.inst->setActor(actor);
        bool ok = false;
        std::string error;
        try {
            if (action == "start")
                ok = l.engine->startWorkItem(l.inst, item);
            else if (action == "complete")
                ok = l.engine->completeWorkItem(l.inst, item, body.value("outcome", "done"));
            else if (action == "reject")
                ok = l.engine->rejectWorkItem(l.inst, item, body.value("reason", ""));
            else if (action == "escalate")
                ok = l.engine->escalateWorkItem(l.inst, item, body.value("reason", ""));
            else if (action == "call-service")
                ok = l.engine->callItemService(l.inst, item);
            else if (action == "assign") {
                std::string pid = body.value("participantId", "");
                if (pid.empty()) throw std::runtime_error("participantId is required");
                Participant* chosen = 0;
                const std::vector<Participant*>& pool = l.engine->getParticipants();
                for (std::size_t i = 0; i < pool.size(); ++i)
                    if (pool[i]->getId() == pid) { chosen = pool[i]; break; }
                if (!chosen) throw std::runtime_error("Participant not found: " + pid);
                ok = l.engine->assignWorkItem(l.inst, item, chosen);
            }

            if (!ok) {
                error = l.engine->getLastError();
                if (error.empty()) error = "Action refused";
            }
        } catch (const std::exception& e) {
            error = e.what();
        }

        l.inst->setActor(0);

        if (!ok) {
            if (checkpointed) delete store.undo();   // discard useless checkpoint
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", error}}.dump(),
                            "application/json");
            return;
        }

        res.set_content(json{{"ok", true}}.dump(), "application/json");
    });

    // ==================== Set instance data ====================
    svr.Post(R"(/api/instances/([^/]+)/data)",
             [this](const httplib::Request& req, httplib::Response& res) {
        Located l = locate(session, req.matches[1].str());
        if (!l.ok()) { res.status = 404; return; }
        try {
            auto body = json::parse(req.body);
            if (!body.is_object())
                throw std::runtime_error("Expected a JSON object of key/value pairs");
            for (auto it = body.begin(); it != body.end(); ++it) {
                if (it.value().is_string())
                    l.inst->setData(it.key(), it.value().get<std::string>());
                else
                    l.inst->setData(it.key(), it.value().dump());
            }
            res.set_content(json{{"ok", true}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                            "application/json");
        }
    });

    // ==================== Advance simulated time ====================
    svr.Post("/api/engine/advance-clock", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            json body;
            if (req.body.empty()) body = json{{"days", 1}};
            else                   body = json::parse(req.body);
            long days = body.value("days", 1);
            if (days < 1) days = 1;
            long seconds = days * 24L * 3600L;
            int total = 0;
            const std::vector<std::string>& ids = session.ids();
            for (std::size_t i = 0; i < ids.size(); ++i) {
                Workspace* ws = session.find(ids[i]);
                if (ws) total += ws->getEngine().advanceClock(seconds);
            }
            res.set_content(json{{"ok", true},
                                 {"escalated", total},
                                 {"days", days}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                            "application/json");
        }
    });

    // ==================== Systems + outage toggle ====================
    svr.Get(R"(/api/scenarios/([^/]+)/systems)",
            [this](const httplib::Request& req, httplib::Response& res) {
        Workspace* ws = session.find(req.matches[1].str());
        if (!ws) { res.status = 404; return; }
        json arr = json::array();
        std::vector<std::string> sys = ws->getScenario().systems();
        for (std::size_t i = 0; i < sys.size(); ++i) arr.push_back(sys[i]);
        res.set_content(json{{"ok", true}, {"systems", arr}}.dump(), "application/json");
    });

    svr.Post(R"(/api/scenarios/([^/]+)/systems/([^/]+)/outage)",
             [this](const httplib::Request& req, httplib::Response& res) {
        Workspace* ws = session.find(req.matches[1].str());
        if (!ws) { res.status = 404; return; }
        try {
            json body;
            if (req.body.empty()) body = json{{"down", true}};
            else                   body = json::parse(req.body);
            bool down = body.value("down", true);
            std::string sys = req.matches[2].str();
            bool ok = ws->getScenario().setOutage(sys, down);
            res.status = ok ? 200 : 400;
            res.set_content(json{{"ok", ok},
                                 {"system", sys},
                                 {"down", down}}.dump(),
                            "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"ok", false}, {"error", e.what()}}.dump(),
                            "application/json");
        }
    });
}

void ApiServer::listen() {
    setupRoutes();
    std::cout << "Server listening on http://localhost:" << port << "\n";
    svr.listen("0.0.0.0", port);
}

}  // namespace api