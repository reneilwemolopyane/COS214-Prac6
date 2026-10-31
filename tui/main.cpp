/**
 * @file main.cpp
 * @brief TIVIDY interactive text interface.
 *
 * One program, two industries (insurance and education) on the SAME engine classes.
 * Everything the user does goes through WorkflowFacade / WorkflowEngine: starting an instance,
 * seeing available work, taking actions, observing routing and state changes, history, undo.
 * Works with a keyboard or with piped input (e.g. `./tividy_tui < tests/tui_smoke.txt`).
 */
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#  include <io.h>
#  define TIVIDY_ISATTY(fd) _isatty(fd)
#else
#  include <unistd.h>
#  define TIVIDY_ISATTY(fd) isatty(fd)
#endif

#include "tividy/scenarios/Session.h"
#include "tividy/scenarios/ItemActions.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Participant.h"
#include "tividy/observer/Observers.h"
#include "tividy/mediator/InstanceCoordinator.h"
#include "tividy/memento/ProcessDataStore.h"
#include "tividy/visitor/AuditReportVisitor.h"
#include "tividy/visitor/BottleneckVisitor.h"

namespace {

struct EndOfInput {};
bool useColor = false;

std::string c(const char* code, const std::string& s) { return useColor ? std::string("\033[") + code + "m" + s + "\033[0m" : s; }
std::string bold(const std::string& s)  { return c("1", s); }
std::string dim(const std::string& s)   { return c("2", s); }
std::string green(const std::string& s) { return c("32", s); }
std::string red(const std::string& s)   { return c("31", s); }
std::string amber(const std::string& s) { return c("33", s); }
std::string cyan(const std::string& s)  { return c("36", s); }

std::string ask(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw EndOfInput();
    if (!TIVIDY_ISATTY(0)) std::cout << line << "\n";    // echo piped input so transcripts read naturally
    std::size_t a = line.find_first_not_of(" \t\r"), b = line.find_last_not_of(" \t\r");
    return a == std::string::npos ? std::string() : line.substr(a, b - a + 1);
}

int askNumber(const std::string& prompt) {
    std::string s = ask(prompt);
    if (s.empty()) return -1;
    for (std::size_t i = 0; i < s.size(); ++i) if (s[i] < '0' || s[i] > '9') return -1;
    return std::atoi(s.c_str());
}

std::string pad(const std::string& s, std::size_t w) { return s.size() >= w ? s.substr(0, w) : s + std::string(w - s.size(), ' '); }

std::string colourState(const std::string& st) {
    if (st == "Completed") return green(pad(st, 10));
    if (st == "Rejected" || st == "Cancelled") return red(pad(st, 10));
    if (st == "Escalated" || st == "Suspended") return amber(pad(st, 10));
    if (st == "Skipped") return dim(pad(st, 10));
    if (st == "Created") return dim(pad(st, 10));
    return cyan(pad(st, 10));
}

std::string outcomeBadge(const std::string& o) {
    if (o == "COMPLETED") return green("[" + o + "]");
    if (o == "REJECTED" || o == "CANCELLED") return red("[" + o + "]");
    if (o == "IN_PROGRESS") return cyan("[" + o + "]");
    return amber("[" + o + "]");
}

void rule() { std::cout << dim(std::string(78, '-')) << "\n"; }

// ---------- the running session ----------
// Session (scenarios/Session.h) holds both industries' workspaces and the undo checkpoints.
// The TUI only remembers which industry is selected.
struct View {
    Session session;
    Workspace* current;
    std::vector<std::string> order;
    View() : current(0) { order = session.ids(); current = session.find(order[0]); }
    ProcessDataStore& storeFor(const std::string& id) { return session.storeFor(id); }
};

void printItems(WorkflowInstance* in) {
    const std::vector<WorkItem*>& items = in->getCoordinator()->getItems();
    std::cout << bold("  #  " + pad("Work item", 30) + pad("Role", 24) + pad("State", 11) + "Assignee") << "\n";
    for (std::size_t i = 0; i < items.size(); ++i) {
        WorkItem* it = items[i];
        std::ostringstream n; n << (i + 1);
        std::string who = it->getAssignee() ? it->getAssignee()->getName() : std::string("-");
        std::cout << "  " << pad(n.str(), 3) << pad(it->getName(), 30) << pad(it->getRole(), 24) << colourState(it->getStateName()) << " " << who << "\n";
    }
}

void printNewEvents(WorkflowInstance* in, std::size_t from) {
    const std::vector<WorkflowEvent>& trail = in->getAuditLogger()->getTrail();
    if (from >= trail.size()) return;
    std::cout << dim("  what happened:") << "\n";
    for (std::size_t i = from; i < trail.size(); ++i) {
        std::string line = AuditLogger::format(trail[i]);
        EventType t = trail[i].type;
        if (t == EVT_REFUSED || t == EVT_FAILED || t == EVT_REJECTED || t == EVT_CANCELLED) line = red(line);
        else if (t == EVT_SKIPPED) line = dim(line);
        else if (t == EVT_ROUTED || t == EVT_ESCALATED) line = amber(line);
        else if (t == EVT_COMPLETED) line = green(line);
        std::cout << "    " << line << "\n";
    }
}

void header(View& s, WorkflowInstance* in) {
    rule();
    std::cout << bold(s.current->getScenario().industry()) << " | " << in->getId() << "  "
              << outcomeBadge(s.current->getEngine().getOutcome(in->getId()))
              << dim("  engine: " + s.current->getEngine().getStatus(in->getId())) << "\n";
    rule();
}

WorkItem* pickItem(WorkflowInstance* in, const std::string& prompt) {
    const std::vector<WorkItem*>& items = in->getCoordinator()->getItems();
    int n = askNumber(prompt);
    if (n < 1 || static_cast<std::size_t>(n) > items.size()) { std::cout << "  no such item\n"; return 0; }
    return items[n - 1];
}

// Acts as the item's assignee unless the user chose someone else.
void actAs(WorkflowInstance* in, WorkItem* it, Participant* override) {
    in->setActor(override ? override : it->getAssignee());
}

void workOnItem(View& s, WorkflowInstance* in) {
    WorkItem* it = pickItem(in, "  item number: ");
    if (!it) return;
    WorkflowEngine& engine = s.current->getEngine();
    Participant* other = 0;
    for (;;) {
        std::cout << "\n  " << bold(it->getName()) << "  " << colourState(it->getStateName()) << " assignee: "
                  << (it->getAssignee() ? it->getAssignee()->getName() : "-")
                  << (other ? "   acting as: " + other->getName() : std::string()) << "\n";
        std::string approval = it->getData("approval.level");
        if (!approval.empty()) std::cout << "  approval level: " << approval << "\n";
        std::cout << "   1) start   2) complete   3) reject   4) escalate (no response)   5) act as someone else   6) call its external system   0) back\n";
        int choice = askNumber("  > ");
        if (choice == 0 || choice < 0) return;
        if (choice == 6) {
            ItemConfig cfg = in->getItemConfig(it->getId());
            if (cfg.serviceConnector.empty()) { std::cout << "  this item has no external system\n"; continue; }
            std::size_t before = in->getAuditLogger()->size();
            bool ok = engine.callItemService(in, it);
            std::cout << "  " << (ok ? green("OK") : red("FAILED")) << " " << cfg.serviceConnector << " " << cfg.serviceOperation
                      << (ok ? "" : ": " + engine.getLastError()) << "\n";
            printNewEvents(in, before);
            std::string answer = in->getData(it->getId() + ".valid");
            if (!answer.empty()) std::cout << "  answer: valid = " << (answer == "true" ? green(answer) : red(answer)) << "\n";
            continue;
        }
        if (choice == 5) {
            const std::vector<Participant*>& ps = engine.getParticipants();
            for (std::size_t i = 0; i < ps.size(); ++i)
                std::cout << "   " << (i + 1) << ") " << ps[i]->getName() << dim("  (" + *ps[i]->getRoles().begin() + ")") << "\n";
            int p = askNumber("  person (0 = the assignee): ");
            other = (p >= 1 && static_cast<std::size_t>(p) <= ps.size()) ? ps[p - 1] : 0;
            continue;
        }
        std::size_t before = in->getAuditLogger()->size();
        s.storeFor(in->getId()).append(in->createMemento());           // checkpoint so the step can be rolled back
        actAs(in, it, other);
        // menu number -> action name; the lookup table lives in ItemActions (shared with the web API)
        static const char* const kMenu[] = { "", "start", "complete", "reject", "escalate" };
        if (choice < 1 || choice > 4) { std::cout << "  ?\n"; continue; }
        const std::string label = kMenu[choice];
        const std::string argLabel = ItemActions::argumentLabel(label);
        std::string argument;
        if (!argLabel.empty()) argument = ask("  " + argLabel + (label == "complete" ? " (e.g. REPAIR, WRITE_OFF, CLEAR, SUSPICIOUS, or Enter for 'done'): " : " (Enter for the default): "));
        bool ok = ItemActions::run(label, engine, in, it, argument);
        in->setActor(0);
        if (ok) std::cout << "  " << green("OK") << " " << label << "\n";
        else {
            delete s.storeFor(in->getId()).undo();                       // nothing changed: drop the useless checkpoint
            std::cout << "  " << red("REFUSED") << " " << label << ": " << engine.getLastError() << "\n";
        }
        printNewEvents(in, before);
        if (in->isFinished() || s.current->getEngine().getStatus(in->getId()) != "RUNNING") return;
    }
}

void provideData(WorkflowInstance* in) {
    std::string key = ask("  data key (e.g. policeCaseNumber): ");
    if (key.empty()) return;
    std::string value = ask("  value: ");
    in->setData(key, value);
    std::cout << "  " << green("saved") << " " << key << " = " << value << dim("   (guards and approval chains read this)") << "\n";
}

void showSideEffects(WorkflowInstance* in) {
    std::cout << bold("  Notifications sent") << "\n";
    const std::vector<std::string>& m = in->getNotificationService()->getMessages();
    if (m.empty()) std::cout << dim("    none yet") << "\n";
    for (std::size_t i = 0; i < m.size(); ++i) std::cout << "    " << m[i] << "\n";
    std::cout << bold("  Incidents (retries exhausted)") << "\n";
    const std::vector<std::string>& inc = in->getIncidentMonitor()->getIncidents();
    if (inc.empty()) std::cout << dim("    none") << "\n";
    for (std::size_t i = 0; i < inc.size(); ++i) std::cout << "    " << red(inc[i]) << "\n";
    std::cout << bold("  Decorator log (audit / notification decorators)") << "\n";
    const std::vector<std::string>& d = in->getDecoratorLog();
    if (d.empty()) std::cout << dim("    none yet") << "\n";
    for (std::size_t i = 0; i < d.size(); ++i) std::cout << "    " << d[i] << "\n";
    std::cout << bold("  Commands run (Command pattern)") << "\n";
    const std::vector<WorkflowCommand*>& cmds = in->getProcessor()->getHistory();
    for (std::size_t i = 0; i < cmds.size(); ++i) {
        std::ostringstream l; l << "    " << cmds[i]->name() << " " << cmds[i]->getWorkItemId() << "  [" << cmds[i]->getStatus() << ", attempts " << cmds[i]->getAttempts() << "]";
        std::cout << l.str() << "\n";
    }
}

// Simulated time: any assigned item whose deadline has passed is escalated by the engine.
void letTimePass(View& s, WorkflowInstance* in) {
    int days = askNumber("  how many days pass? ");
    if (days < 1) return;
    WorkflowEngine& engine = s.current->getEngine();
    std::size_t before = in ? in->getAuditLogger()->size() : 0;
    int moved = engine.advanceClock(static_cast<long>(days) * 24 * 3600);
    std::cout << "  " << days << " day(s) pass (clock = day " << (engine.getClock() / 86400) << "): "
              << (moved ? amber(std::to_string(moved) + " item(s) escalated") : std::string("no deadline expired")) << "\n";
    if (in) printNewEvents(in, before);
}

void instanceMenu(View& s, WorkflowInstance* in) {
    WorkflowEngine& engine = s.current->getEngine();
    for (;;) {
        header(s, in);
        printItems(in);
        std::cout << "\n  1) do work on an item     2) history (audit trail)    3) provide / change data\n"
                     "  4) notifications, incidents, commands                5) progress reports (visitors)\n"
                     "  6) undo last step (rollback to checkpoint)           7) cancel instance (reverse payments)\n"
                     "  8) show instance data     9) let time pass (deadlines)     0) back\n";
        int choice = askNumber("  > ");
        if (choice <= 0) return;
        switch (choice) {
        case 1: workOnItem(s, in); break;
        case 2: { std::vector<std::string> lines = in->getAuditLogger()->getLines();
                  for (std::size_t i = 0; i < lines.size(); ++i) std::cout << "  " << lines[i] << "\n"; } break;
        case 3: provideData(in); break;
        case 4: showSideEffects(in); break;
        case 5: { AuditReportVisitor audit; in->accept(audit); std::cout << audit.getReport() << "\n";
                  BottleneckVisitor slow; in->accept(slow); std::cout << slow.getReport() << "\n"; } break;
        case 6: { InstanceMemento* m = s.storeFor(in->getId()).undo();
                  if (!m) { std::cout << "  nothing to undo\n"; break; }
                  in->restoreFromMemento(*m); delete m;
                  in->raiseEvent(EVT_ROUTED, "", 0, "rolled back to the previous checkpoint (history is kept)");
                  std::cout << "  " << green("rolled back") << " one step. The audit trail keeps everything that happened.\n"; } break;
        case 7: { std::size_t before = in->getAuditLogger()->size();
                  if (engine.cancel(in->getId())) std::cout << "  " << amber("cancelled") << ": completed external calls were reversed\n";
                  else std::cout << "  " << red("cannot cancel") << " (not running)\n";
                  printNewEvents(in, before); } break;
        case 8: { const DataMap& d = in->getAllData();
                  for (DataMap::const_iterator it = d.begin(); it != d.end(); ++it) std::cout << "  " << pad(it->first, 28) << it->second << "\n"; } break;
        case 9: letTimePass(s, in); break;
        default: std::cout << "  ?\n";
        }
    }
}

void startInstance(View& s) {
    Scenario& sc = s.current->getScenario();
    std::cout << "\n" << bold(sc.title()) << "\n  " << sc.summary() << "\n\n";
    std::vector<SampleCase> cases = sc.sampleCases();
    for (std::size_t i = 0; i < cases.size(); ++i)
        std::cout << "  " << (i + 1) << ") " << bold(cases[i].label) << "\n     " << dim(cases[i].story) << "\n";
    std::cout << "  " << (cases.size() + 1) << ") custom data\n";
    int n = askNumber("  case > ");
    if (n < 1 || static_cast<std::size_t>(n) > cases.size() + 1) return;
    DataMap data;
    if (static_cast<std::size_t>(n) == cases.size() + 1) {
        std::cout << "  enter key=value lines, blank line to finish\n";
        for (;;) { std::string l = ask("  "); if (l.empty()) break; std::size_t eq = l.find('='); if (eq != std::string::npos) data[l.substr(0, eq)] = l.substr(eq + 1); }
    } else data = cases[n - 1].data;
    try {
        std::string id = s.current->getFacade().startWorkflow(sc.id(), data);
        WorkflowInstance* in = s.current->getEngine().findInstance(id);
        std::cout << "  " << green("started") << " " << id << "\n";
        printNewEvents(in, 0);
        instanceMenu(s, in);
    } catch (const std::exception& e) { std::cout << "  " << red("could not start: ") << e.what() << "\n"; }
}

void listInstances(View& s) {
    const std::vector<WorkflowInstance*>& v = s.current->getEngine().getInstances();
    if (v.empty()) { std::cout << "  no instances yet\n"; return; }
    for (std::size_t i = 0; i < v.size(); ++i)
        std::cout << "  " << (i + 1) << ") " << v[i]->getId() << " " << outcomeBadge(s.current->getEngine().getOutcome(v[i]->getId())) << "\n";
    int n = askNumber("  open (0 = back) > ");
    if (n >= 1 && static_cast<std::size_t>(n) <= v.size()) instanceMenu(s, v[n - 1]);
}

void outages(View& s) {
    Scenario& sc = s.current->getScenario();
    std::vector<std::string> sys = sc.systems();
    for (std::size_t i = 0; i < sys.size(); ++i) std::cout << "  " << (i + 1) << ") " << sys[i] << "\n";
    int n = askNumber("  system > ");
    if (n < 1 || static_cast<std::size_t>(n) > sys.size()) return;
    std::string st = ask("  down or up? ");
    sc.setOutage(sys[n - 1], st == "down");
    std::cout << "  " << sys[n - 1] << " is now " << (st == "down" ? red("DOWN") : green("UP")) << dim("  (the stub adapter reports temporary failures, so the engine retries and then raises an incident)") << "\n";
}

void switchIndustry(View& s) {
    for (std::size_t i = 0; i < s.order.size(); ++i) {
        Workspace* w = s.session.find(s.order[i]);
        std::cout << "  " << (i + 1) << ") " << bold(w->getScenario().industry()) << " - " << w->getScenario().title() << "\n";
    }
    int n = askNumber("  industry > ");
    if (n >= 1 && static_cast<std::size_t>(n) <= s.order.size()) s.current = s.session.find(s.order[n - 1]);
}

void mainMenu(View& s) {
    for (;;) {
        std::cout << "\n" << bold("TIVIDY") << " workflow management   " << dim("one engine, two industries") << "\n";
        std::cout << "Industry: " << bold(s.current->getScenario().industry()) << " - " << s.current->getScenario().title() << "\n";
        std::cout << "  1) start a new workflow instance\n  2) open an existing instance\n"
                     "  3) switch industry\n  4) switch an external system on/off (retry & incident demo)\n  5) let time pass (deadlines escalate)\n  0) quit\n";
        int n = askNumber("> ");
        if (n == 0) return;
        if (n == 1) startInstance(s);
        else if (n == 2) listInstances(s);
        else if (n == 3) switchIndustry(s);
        else if (n == 4) outages(s);
        else if (n == 5) letTimePass(s, 0);
        else if (n < 0) continue;
    }
}

} // namespace

int main(int argc, char** argv) {
    useColor = TIVIDY_ISATTY(1) != 0;
    for (int i = 1; i < argc; ++i) if (std::string(argv[i]) == "--no-color") useColor = false;
    try {
        View view;
        mainMenu(view);
    } catch (const EndOfInput&) {
        std::cout << "\n";
    }
    return 0;
}
