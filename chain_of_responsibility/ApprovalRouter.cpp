#include "tividy/chain_of_responsibility/ApprovalRouter.h"
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace {
std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
}

ApprovalRouter* ApprovalRouter::parse(const std::string& levels) {
    ApprovalRouter* r = new ApprovalRouter();
    try {
        std::size_t start = 0;
        while (start <= levels.size()) {
            std::size_t end = levels.find(';', start);
            std::string part = trim(levels.substr(start, end == std::string::npos ? std::string::npos : end - start));
            if (!part.empty()) {
                std::size_t eq = part.find('=');
                if (eq == std::string::npos) throw std::invalid_argument("approval level needs 'Name=limit': " + part);
                std::string name = trim(part.substr(0, eq)), lim = trim(part.substr(eq + 1));
                double limit;
                if (lim == "*") limit = std::numeric_limits<double>::max();
                else {
                    char* e = 0; limit = std::strtod(lim.c_str(), &e);
                    if (lim.empty() || !e || *e != '\0') throw std::invalid_argument("approval limit is not a number or '*': " + part);
                }
                if (name.empty()) throw std::invalid_argument("approval level has no name: " + part);
                ApprovalHandler* h = new LevelApprover(name, limit);
                if (!r->handlers.empty()) r->handlers.back()->setSuccessor(h);
                r->handlers.push_back(h);
                r->names.push_back(name);
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
        if (r->handlers.empty()) throw std::invalid_argument("approval chain has no levels");
    } catch (...) { delete r; throw; }
    return r;
}

ApprovalRouter::~ApprovalRouter() {
    for (std::size_t i = 0; i < handlers.size(); ++i) delete handlers[i];
}

int ApprovalRouter::indexOf(const std::string& name) const {
    for (std::size_t i = 0; i < names.size(); ++i) if (names[i] == name) return static_cast<int>(i);
    return -1;
}

int ApprovalRouter::route(double amount, int fromLevel, std::string& note) {
    note.clear();
    ApprovalRequest req;
    req.amount = amount;
    std::size_t first = 0;
    if (fromLevel >= 0) {                       // that level timed out: its successor gets a fresh deadline
        if (static_cast<std::size_t>(fromLevel) >= handlers.size()) return -1;
        req.deadlinePassed = true;
        first = static_cast<std::size_t>(fromLevel);
    }
    std::string trail;
    handlers[first]->setEscalationListener([&trail](const std::string& s) { if (!trail.empty()) trail += "; "; trail += s; });
    // Every level reports upward through the same listener so the audit line shows the whole climb.
    for (std::size_t i = first + 1; i < handlers.size(); ++i)
        handlers[i]->setEscalationListener([&trail](const std::string& s) { if (!trail.empty()) trail += "; "; trail += s; });
    ApprovalOutcome outcome = handlers[first]->handleRequest(req);
    for (std::size_t i = first; i < handlers.size(); ++i) handlers[i]->setEscalationListener(ApprovalHandler::EscalationListener());
    note = trail;
    if (outcome != APPROVED) return -1;
    return indexOf(req.approvedBy);
}
