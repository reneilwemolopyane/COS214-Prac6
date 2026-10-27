#pragma once
#include <string>
#include <vector>
#include "tividy/composite_prototype/WorkflowComponent.h"

/** Minimal WorkflowComponent for tests. Records every call into a shared log. */
class StubItem : public WorkflowComponent {
public:
    StubItem(const std::string& n, std::vector<std::string>* l)
        : name(n), log(l), done(false), cancelled(false) {}

    void start()    { record("start"); }
    void activate() override { record("activate"); }
    void complete() override {
        record("complete");
        done = true;
        if (getParent()) getParent()->childFinished(*this);
    }
    void cancel()   { record("cancel"); cancelled = true; }
    bool isComplete() const { return done; }
    bool isFinished() const override { return done || cancelled; }
    bool isFailed() const override { return false; }
    std::string getName() const { return name; }
    void accept(WorkflowVisitor&) {}

    /** Fresh copy: same name and log, but not completed. */
    WorkflowComponent* clone() const { return new StubItem(name, log); }

private:
    void record(const std::string& op) { if (log) log->push_back(op + ":" + name); }
    std::string name;
    std::vector<std::string>* log;
    bool done, cancelled;
};