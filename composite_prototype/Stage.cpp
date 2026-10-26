#include "tividy/composite_prototype/Stage.h"
#include "tividy/state/InvalidTransitionException.h"
#include <stdexcept>
#include <memory>

Stage::Stage(const std::string& n, ExecutionMode m)
    : name(n), mode(m), cursor(0), cancelled(false) {}

Stage::~Stage() {
    for (std::size_t i = 0; i < children.size(); ++i) delete children[i];
}

void Stage::add(WorkflowComponent* c) {
    if (c == nullptr) throw std::invalid_argument("Stage::add(nullptr)");
    children.push_back(c);
}

void Stage::start() {
    if (cancelled) return;
    if (mode == SEQUENTIAL) {
        while (cursor < children.size() && children[cursor]->isComplete()) ++cursor;
        if (cursor < children.size()) children[cursor]->start();
    } else {
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (!children[i]->isComplete()) children[i]->start();
        }
    }
}

void Stage::complete() { /* no-op: derived from children */ }

void Stage::cancel() {
    if (cancelled) return;
    cancelled = true;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i]->isComplete()) continue;
        try { children[i]->cancel(); }
        catch (const InvalidTransitionException&) { /* already in a final state */ }
    }
}

bool Stage::isComplete() const {
    if (cancelled) return false;
    for (std::size_t i = 0; i < children.size(); ++i)
        if (!children[i]->isComplete()) return false;
    return true;
}

WorkflowComponent* Stage::clone() const {
    std::unique_ptr<Stage> copy(new Stage(name, mode));
    for (std::size_t i = 0; i < children.size(); ++i) {
        std::unique_ptr<WorkflowComponent> c(children[i]->clone());
        copy->add(c.get());
        c.release();
    }
    return copy.release();
}

std::string Stage::getName() const { return name; }

void Stage::accept(WorkflowVisitor& v) {
    // Pre-order: self-visit deferred until Visitor.h is provided.
    for (std::size_t i = 0; i < children.size(); ++i) children[i]->accept(v);
}

SubProcess::SubProcess(const std::string& n) : Stage(n, SEQUENTIAL) {}

WorkflowComponent* SubProcess::clone() const {
    std::unique_ptr<SubProcess> copy(new SubProcess(name));
    for (std::size_t i = 0; i < children.size(); ++i) {
        std::unique_ptr<WorkflowComponent> c(children[i]->clone());
        copy->add(c.get());
        c.release();
    }
    return copy.release();
}