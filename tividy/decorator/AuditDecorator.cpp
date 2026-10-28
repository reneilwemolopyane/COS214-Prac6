#include "tividy/decorator/AuditDecorator.h"

AuditDecorator::AuditDecorator(WorkflowComponent* w, Sink s)
    : WorkItemDecorator(w), sink(s) {}

void AuditDecorator::log(const std::string& op) {
    std::string entry = op + ": " + wrapped->getName();
    trail.push_back(entry);
    if (sink) sink(entry);
}

void AuditDecorator::start() {
    log("start");
    try { wrapped->start(); }
    catch (...) { log("start FAILED"); throw; }
}

void AuditDecorator::complete() {
    log("complete");
    try { wrapped->complete(); }
    catch (...) { log("complete FAILED"); throw; }
}

void AuditDecorator::complete(const std::string& outcome) {
    log("complete(" + outcome + ")");
    try { wrapped->complete(outcome); }
    catch (...) { log("complete(" + outcome + ") FAILED"); throw; }
}

void AuditDecorator::cancel() {
    log("cancel");
    try { wrapped->cancel(); }
    catch (...) { log("cancel FAILED"); throw; }
}

void AuditDecorator::assign(Participant* p) {
    log("assign");
    try { wrapped->assign(p); }
    catch (...) { log("assign FAILED"); throw; }
}

const std::vector<std::string>& AuditDecorator::history() const { return trail; }

WorkflowComponent* AuditDecorator::clone() const {
    return new AuditDecorator(wrapped->clone(), sink);
}