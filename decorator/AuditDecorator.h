#pragma once
#include <functional>
#include <string>
#include <vector>
#include "WorkItemDecorator.h"

/** @brief Records every lifecycle call so the audit trail can be inspected.
 *  Failed calls are recorded too. */
class AuditDecorator : public WorkItemDecorator {
public:
    /** Receives each audit entry, e.g. to feed the engine's shared history. */
    typedef std::function<void(const std::string&)> Sink;

    /** @param w wrapped component
     *  @param s optional sink that also receives every entry */
    explicit AuditDecorator(WorkflowComponent* w, Sink s = Sink());

    void start();
    void complete();
    void complete(const std::string& outcome) override;
    void cancel();
    void assign(Participant* p) override;

    /** @return the entries recorded by this decorator */
    const std::vector<std::string>& history() const;

    /** Clones the stack with an empty trail, so cloned instances stay isolated. */
    WorkflowComponent* clone() const;
private:
    void log(const std::string& op);
    std::vector<std::string> trail;
    Sink sink;
};