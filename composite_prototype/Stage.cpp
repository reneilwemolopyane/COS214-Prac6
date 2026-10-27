#include "tividy/composite_prototype/Stage.h"
#include <stdexcept>
#include <memory>

Stage::Stage(const std::string& n, ExecutionMode m)
    : name(n), mode(m), activated(false), cancelledFlag(false), 
      finishedNotified(false), busy(false), cursor(0), cursorActivated(false) {}

Stage::~Stage() {
    for (std::size_t i = 0; i < children.size(); ++i) delete children[i];
}

void Stage::add(WorkflowComponent* c) {
    if (!c) throw std::invalid_argument("Stage::add(nullptr)");
    if (activated || cancelledFlag) throw std::logic_error("Cannot add to running/cancelled stage");
    if (c->getParent()) throw std::invalid_argument("Component already has a parent");
    
    children.push_back(c);
    c->setParent(this);
}

void Stage::remove(WorkflowComponent* c) {
    if (activated || cancelledFlag) throw std::logic_error("Cannot remove from running/cancelled stage");
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i] == c) {
            c->setParent(nullptr);
            children.erase(children.begin() + i);
            delete c;
            return;
        }
    }
    throw std::invalid_argument("Component is not a child of this stage");
}

std::size_t Stage::childCount() const { return children.size(); }

WorkflowComponent* Stage::childAt(std::size_t index) const {
    if (index >= children.size()) throw std::out_of_range("Stage::childAt out of range");
    return children[index];
}

// --- Activation & Push Model ---

void Stage::start() { activate(); }

void Stage::activate() {
    if (activated || cancelledFlag) throw std::logic_error("Stage already activated or cancelled");
    activated = true;
    cursor = 0;
    cursorActivated = false;

    {
        BusyGuard g(busy);
        if (mode == PARALLEL) {
            for (std::size_t i = 0; i < children.size(); ++i) {
                children[i]->activate();
            }
        }
    } // busy is false here
    
    advance();
}

void Stage::advance() {
    BusyGuard g(busy);

    if (mode == SEQUENTIAL) {
        while (cursor < children.size()) {
            WorkflowComponent* c = children[cursor];
            if (!cursorActivated) {
                cursorActivated = true;
                c->activate();
            }
            if (!c->isFinished()) return; 
            if (!c->isComplete()) return; 
            ++cursor;
            cursorActivated = false;
        }
    }

    if (isFinished()) finish();
}

void Stage::childFinished(WorkflowComponent& child) {
    (void)child;
    if (!activated || busy) return;
    advance();
}

void Stage::finish() {
    if (finishedNotified) return;
    finishedNotified = true;
    if (getParent()) getParent()->childFinished(*this);
}

void Stage::cancel() {
    if (isFinished()) return; 

    {
        BusyGuard g(busy);
        for (std::size_t i = 0; i < children.size(); ++i) {
            if (!children[i]->isFinished()) {
                children[i]->cancel();
            }
        }
    }
    
    cancelledFlag = true;
    if (isFinished()) finish();
}

void Stage::complete() {
    throw std::logic_error("Stages complete automatically when children finish");
}


bool Stage::isFinished() const {
    if (!activated && !cancelledFlag) return false;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (!children[i]->isFinished()) return false;
    }
    return true;
}

bool Stage::isComplete() const {
    if (!isFinished()) return false;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (!children[i]->isComplete()) return false;
    }
    return true;
}

bool Stage::isFailed() const {
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i]->isFailed()) return true;
    }
    return false;
}


WorkflowComponent* Stage::clone() const {
    std::unique_ptr<Stage> copy(new Stage(name, mode));
    cloneChildrenInto(*copy);
    return copy.release();
}

void Stage::cloneChildrenInto(Stage& target) const {
    for (std::size_t i = 0; i < children.size(); ++i) {
        std::unique_ptr<WorkflowComponent> c(children[i]->clone());
        target.add(c.get());
        c.release();
    }
}

std::string Stage::getName() const { return name; }

void Stage::accept(WorkflowVisitor& v) {
    v.visitStage(*this);
    for (std::size_t i = 0; i < children.size(); ++i) {
        children[i]->accept(v);
    }
}

SubProcess::SubProcess(const std::string& n, ExecutionMode m) : Stage(n, m) {}

WorkflowComponent* SubProcess::clone() const {
    std::unique_ptr<SubProcess> copy(new SubProcess(name, mode));
    cloneChildrenInto(*copy);
    return copy.release();
}