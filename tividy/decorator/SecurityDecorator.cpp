#include "tividy/decorator/SecurityDecorator.h"
#include <stdexcept>

SecurityDecorator::SecurityDecorator(WorkflowComponent* w, const std::string& required)
    : WorkItemDecorator(w), requiredRole(required) {}

SecurityDecorator::SecurityDecorator(WorkflowComponent* w, const std::string& required,
                                     const std::string& actor)
    : WorkItemDecorator(w), requiredRole(required), actorRole(actor) {}

void SecurityDecorator::setActor(const std::string& role) { actorRole = role; }

void SecurityDecorator::setActorProvider(ActorProvider p) { provider = p; }

void SecurityDecorator::check() const {
    std::string who = provider ? provider() : actorRole;
    if (who.empty() || who != requiredRole)
        throw std::runtime_error("Role '" + who +
                                 "' not permitted (needs '" + requiredRole + "')");
}

void SecurityDecorator::start()    { check(); wrapped->start(); }
void SecurityDecorator::complete() { check(); wrapped->complete(); }

void SecurityDecorator::complete(const std::string& outcome) {
    check();
    wrapped->complete(outcome);
}

void SecurityDecorator::assign(Participant* p) {
    check();
    wrapped->assign(p);
}

void SecurityDecorator::cancel()   { check(); wrapped->cancel(); }

WorkflowComponent* SecurityDecorator::clone() const {
    SecurityDecorator* c = new SecurityDecorator(wrapped->clone(), requiredRole, actorRole);
    c->provider = provider;
    return c;
}