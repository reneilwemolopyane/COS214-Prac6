#pragma once
#include <deque>
#include <string>
#include <vector>
#include "doctest.h"
#include "tividy/facade/WorkflowFacade.h"
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/command/WorkflowEngine.h"
#include "tividy/builder/ValidatingWorkflowBuilder.h"
#include "tividy/builder/WorkflowDefinition.h"
#include "tividy/composite_prototype/Stage.h"
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/core/Participant.h"
#include "tividy/core/WorkflowInstance.h"
#include "tividy/observer/Observers.h"
#include "tividy/visitor/Visitor.h"

/**
 * @brief Shared setup for integration tests: real Builder, Facade, Engine and Observers, no fakes.
 *
 * Usage: define() a process with the builder, start() it through the Facade,
 * then act on item("name") with real Participants from person().
 */
class IntegrationFixture {
public:
    IntegrationFixture() : facade(repository, engine), instance(0), audit(0) {}

    ~IntegrationFixture() {
        for (std::size_t i = 0; i < logs.size(); ++i) delete logs[i];
    }

    /** @brief Starts a new definition called @p id; add stages and items to the returned builder. */
    WorkflowBuilder& define(const std::string& id) {
        definitionId = id;
        builder.setIdentity(id, 1);
        return builder;
    }

    /** @brief Registers the definition and starts one instance through the Facade. */
    WorkflowInstance* start(const DataMap& data = DataMap()) {
        facade.registerDefinition(builder.getResult());
        instance = startAnother(data);
        audit = logs.back();
        return instance;
    }

    /** @brief Starts another instance of the same definition, with its own AuditLogger. */
    WorkflowInstance* startAnother(const DataMap& data = DataMap()) {
        std::string id = facade.startWorkflow(definitionId, data);
        WorkflowInstance* started = engine.findInstance(id);
        logs.push_back(new AuditLogger(started));
        return started;
    }

    /** @brief A real participant with one role, owned by the fixture. */
    Participant* person(const std::string& id, const std::string& name, const std::string& role) {
        people.push_back(Participant(id, name));
        people.back().addRole(role);
        return &people.back();
    }

    /** @brief The work item called @p name in @p in (default: the first instance), found with a Visitor. */
    WorkItem* item(const std::string& name, WorkflowInstance* in = 0) {
        Finder finder(name);
        (in ? in : instance)->accept(finder);
        return finder.found;
    }

    /** @brief The audit trail of @p in (default: the first instance). */
    const std::vector<WorkflowEvent>& trail(WorkflowInstance* in = 0) const {
        if (!in || in == instance) return audit->getTrail();
        for (std::size_t i = 0; i < logs.size(); ++i)
            if (logs[i]->getTrail().empty() || logs[i]->getTrail()[0].getSource() == in) return logs[i]->getTrail();
        return audit->getTrail();
    }

    /** @brief How many events of @p type for @p itemId (any item if empty) the first instance raised. */
    int count(EventType type, const std::string& itemId = "", const std::string& command = "") const {
        int n = 0;
        const std::vector<WorkflowEvent>& t = audit->getTrail();
        for (std::size_t i = 0; i < t.size(); ++i)
            if (t[i].getType() == type && (itemId.empty() || t[i].getWorkItemId() == itemId) &&
                (command.empty() || t[i].get("command") == command)) ++n;
        return n;
    }

    /** @brief Position of the first matching event in the first instance's trail, or -1. */
    int indexOf(EventType type, const std::string& itemId) const {
        const std::vector<WorkflowEvent>& t = audit->getTrail();
        for (std::size_t i = 0; i < t.size(); ++i)
            if (t[i].getType() == type && t[i].getWorkItemId() == itemId) return (int)i;
        return -1;
    }

    std::deque<Participant> people;
    DefinitionRepository repository;
    WorkflowEngine engine;
    WorkflowFacade facade;
    ValidatingWorkflowBuilder builder;
    std::string definitionId;
    WorkflowInstance* instance;
    AuditLogger* audit;

private:
    struct Finder : public WorkflowVisitor {
        explicit Finder(const std::string& n) : name(n), found(0) {}
        void visitInstance(WorkflowInstance&) {}
        void visitStage(Stage&) {}
        void visitWorkItem(WorkItem& i) { if (i.getId() == name) found = &i; }
        std::string getReport() const { return ""; }
        std::string name;
        WorkItem* found;
    };

    std::vector<AuditLogger*> logs;
};