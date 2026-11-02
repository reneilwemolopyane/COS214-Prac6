#include "tividy/decorator/DecoratorFactory.h"
#include "tividy/decorator/Decorators.h"
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace {
std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
void split(const std::string& spec, std::vector<std::pair<std::string, std::string> >& out) {
    std::size_t start = 0;
    while (start <= spec.size()) {
        std::size_t end = spec.find(',', start);
        std::string tok = trim(spec.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (!tok.empty()) {
            std::size_t c = tok.find(':');
            out.push_back(std::make_pair(trim(tok.substr(0, c)), c == std::string::npos ? std::string() : trim(tok.substr(c + 1))));
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
}
}

void DecoratorFactory::validate(const std::string& spec) {
    std::vector<std::pair<std::string, std::string> > parts;
    split(spec, parts);
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const std::string& n = parts[i].first; const std::string& arg = parts[i].second;
        if (n == "audit" || n == "priority" || n == "notification") continue;
        if (n == "validation" || n == "security" || n == "deadline") {
            if (arg.empty()) throw std::invalid_argument("decorator '" + n + "' needs an argument (" + n + ":<value>)");
            continue;
        }
        throw std::invalid_argument("unknown decorator '" + n + "'");
    }
}

WorkflowComponent* DecoratorFactory::wrap(WorkflowComponent* item, const std::string& spec, const Context& ctx) {
    validate(spec);
    std::vector<std::pair<std::string, std::string> > parts;
    split(spec, parts);
    WorkflowComponent* current = item;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const std::string& n = parts[i].first; const std::string& arg = parts[i].second;
        if (n == "audit") current = new AuditDecorator(current, ctx.audit);
        else if (n == "priority") current = new PriorityDecorator(current, 2);
        else if (n == "notification") current = new NotificationDecorator(current, ctx.notify);
        else if (n == "deadline") current = new DeadlineDecorator(current, std::atol(arg.c_str()));
        else if (n == "validation") {
            std::function<std::string(const std::string&)> read = ctx.data;
            std::string key = arg;
            current = new ValidationDecorator(current, [read, key]() { return read && !read(key).empty(); });
        } else if (n == "security") {
            SecurityDecorator* s = new SecurityDecorator(current, arg);
            std::function<std::string()> who = ctx.actorRole; std::string need = arg;
            s->setActorProvider([who, need]() { return who ? who() : std::string(); });
            current = s;
        }
    }
    return current;
}
