#include "tividy/core/WorkflowInstance.h"
#include "tividy/composite_prototype/Stage.h"  
#include <sstream>
#include "tividy/composite_prototype/WorkItem.h"
#include "tividy/state/States.h"
#include "tividy/observer/Observers.h"
#include "tividy/strategy/WorkAssigner.h"
#include "tividy/strategy/AssignmentStrategy.h"
#include "tividy/command/Command.h"
#include "tividy/core/Participant.h"
#include "tividy/decorator/DecoratorFactory.h"
#include <stdexcept>


class RegistrationVisitor : public WorkflowVisitor {
    InstanceCoordinator* coord;
public:
    RegistrationVisitor(InstanceCoordinator* c) : coord(c) {}
    
    void visitInstance(WorkflowInstance&) override {}
    void visitStage(Stage&) override {}
    
    void visitWorkItem(WorkItem& item) override {
       
        coord->registerItem(&item); 
    }
    
    std::string getReport() const override { return ""; }
};

WorkflowInstance::WorkflowInstance(const std::string& id,
                                   WorkflowComponent* rootCopy,
                                   const DataMap& d,
                                   int version)
    : id(id),
      root(rootCopy),
      data(d),
      version(version),
      coordinator(0),
      processor(new CommandProcessor(this)),
      audit(new AuditLogger()),
      tracker(new DependencyTracker()),
      notifier(new NotificationService()),
      monitor(new IncidentMonitor(processor)),
      assigner(0),
      clockSeconds(0),
      actor(0) {
    // Standard reactions. Whoever creates the instance can attach more.
    attach(audit);                          // every event
    attach(EVT_COMPLETED, tracker);
    attach(EVT_FAILED, tracker);
    attach(EVT_REJECTED, tracker);
    attach(EVT_CANCELLED, tracker);
    attach(EVT_ESCALATED, tracker);
    attach(EVT_TIMED_OUT, tracker);
    attach(EVT_ASSIGNED, notifier);
    attach(EVT_ESCALATED, notifier);
    attach(EVT_FAILED, monitor);
    attach(EVT_TIMED_OUT, monitor);
}

WorkflowInstance::~WorkflowInstance() {
    
    delete root;
    delete coordinator;
    delete assigner;
    delete processor;
    delete audit;
    delete tracker;
    delete notifier;
    delete monitor;
}

void WorkflowInstance::start() {
    if (!root) return;
    delete coordinator;
    delete assigner;
    assigner = new WorkAssigner();
    assigner->setDefaultStrategy(RoleBasedStrategy());
    // Named strategies a definition can pick per item (Strategy pattern).
    assigner->registerStrategy("role", RoleBasedStrategy());
    assigner->registerStrategy("shortest-queue", ShortestQueueStrategy());
    assigner->registerStrategy("round-robin", RoundRobinStrategy());
    coordinator = new InstanceCoordinator(assigner, this);
    coordinator->setData(&data);
    coordinator->setClock(clockSeconds);

    applyDecorations();                       // wrap items before they are registered
    RegistrationVisitor regVisitor(coordinator);
    root->accept(regVisitor);

    // Hand the definition's routing settings to the mediator and the items.
    for (std::map<std::string, ItemConfig>::const_iterator it = itemConfigs.begin(); it != itemConfigs.end(); ++it) {
        const ItemConfig& cfg = it->second;
        WorkItem* item = coordinator->findItem(it->first);
        if (!item) throw std::invalid_argument("Definition configures unknown work item: " + it->first);
        if (!cfg.guard.empty()) coordinator->setGuard(it->first, cfg.guard);
        if (!cfg.approvalLevels.empty()) coordinator->setApproval(it->first, cfg.approvalAmountKey, cfg.approvalLevels);
        if (!cfg.escalationRole.empty()) coordinator->setEscalationRole(it->first, cfg.escalationRole);
        if (!cfg.assignmentRule.empty()) item->setAssignmentRule(cfg.assignmentRule);
        if (cfg.deadlineSeconds > 0) {   // decorators such as Priority may shorten the deadline
            WorkflowComponent* front = frontOf(it->first);
            coordinator->setDeadline(it->first, front ? front->adjustDeadline(cfg.deadlineSeconds) : cfg.deadlineSeconds);
        }
    }
    for (std::size_t i = 0; i < dependencies.size(); ++i)
        coordinator->addDependency(dependencies[i].first, dependencies[i].second);
    for (std::size_t i = 0; i < orderings.size(); ++i)
        coordinator->addOrdering(orderings[i].first, orderings[i].second);
    for (std::size_t i = 0; i < participants.size(); ++i)
        coordinator->addParticipant(participants[i]);

    // The coordinator releases (and assigns) every item whose prerequisites are met.
    coordinator->start();
    
}

ItemConfig WorkflowInstance::getItemConfig(const std::string& itemId) const {
    std::map<std::string, ItemConfig>::const_iterator it = itemConfigs.find(itemId);
    return it == itemConfigs.end() ? ItemConfig() : it->second;
}

std::string WorkflowInstance::getData(const std::string& key) const {
    DataMap::const_iterator it = data.find(key);
    return it == data.end() ? std::string() : it->second;
}

WorkflowComponent* WorkflowInstance::frontOf(const std::string& itemId) const {
    std::map<std::string, WorkflowComponent*>::const_iterator it = fronts.find(itemId);
    return it == fronts.end() ? 0 : it->second;
}

// Wraps each configured item in its decorator stack and remembers the outermost component,
// so commands reach security/validation/audit decorators first (Decorator pattern).
void WorkflowInstance::applyDecorations() {
    fronts.clear();
    std::vector<std::pair<WorkItem*, WorkflowComponent*> > found;
    root->collectItems(found);
    DecoratorFactory::Context ctx;
    ctx.actorRole = [this]() -> std::string {
        if (!actor) return "";
        return actor->getRoles().empty() ? std::string() : *actor->getRoles().begin();
    };
    ctx.data = [this](const std::string& k) { return getData(k); };
    ctx.audit = [this](const std::string& s) { decoratorLog.push_back("audit: " + s); };
    ctx.notify = [this](const std::string& s) { decoratorLog.push_back("notify: " + s); };
    for (std::size_t i = 0; i < found.size(); ++i) {
        WorkItem* item = found[i].first; WorkflowComponent* front = found[i].second;
        std::map<std::string, ItemConfig>::const_iterator cfg = itemConfigs.find(item->getId());
        if (cfg != itemConfigs.end() && !cfg->second.decorations.empty()) {
            // security:<Role> passes when the acting participant holds that role.
            DecoratorFactory::Context perItem = ctx;
            Participant** who = &actor;
            perItem.actorRole = [who, cfg]() -> std::string {
                if (!*who) return "";
                const std::string& spec = cfg->second.decorations;
                std::size_t p = spec.find("security:");
                if (p != std::string::npos) {
                    std::size_t e = spec.find(',', p);
                    std::string need = spec.substr(p + 9, e == std::string::npos ? std::string::npos : e - p - 9);
                    std::size_t a = need.find_first_not_of(" "), b = need.find_last_not_of(" ");
                    need = a == std::string::npos ? "" : need.substr(a, b - a + 1);
                    if ((*who)->hasRole(need)) return need;
                }
                return (*who)->getRoles().empty() ? std::string() : *(*who)->getRoles().begin();
            };
            WorkflowComponent* wrapped = DecoratorFactory::wrap(front, cfg->second.decorations, perItem);
            if (wrapped != front) {
                // The decorator adopted `front`; put it where the item was in the tree.
                if (!root->replaceChild(front, wrapped)) { delete wrapped; throw std::logic_error("could not decorate " + item->getId()); }
                front = wrapped;
            }
        }
        fronts[item->getId()] = front;
    }
}

std::string WorkflowInstance::getOutcome(bool cancelled) const {
    if (cancelled) return "CANCELLED";
    if (!coordinator) return "IN_PROGRESS";
    if (isCompleted()) return "COMPLETED";
    if (isFinished()) {
        const std::vector<WorkItem*>& all = coordinator->getItems();
        for (std::size_t i = 0; i < all.size(); ++i)
            if (all[i]->wasRejected()) return "REJECTED";
        return "CLOSED";
    }
    return "IN_PROGRESS";
}

void WorkflowInstance::addDependency(const std::string& prerequisite, const std::string& item) {
    dependencies.push_back(std::make_pair(prerequisite, item));
}

void WorkflowInstance::addParticipant(Participant* participant) {
    if (participant) participants.push_back(participant);
}

void WorkflowInstance::cancelAll() {
    if (root) root->cancel();
}

bool WorkflowInstance::isFinished() const { return coordinator && coordinator->isFinished(); }

bool WorkflowInstance::isCompleted() const { return coordinator && coordinator->allCompleted(); }

void WorkflowInstance::raiseEvent(EventType type, const std::string& workItemId,
                                  WorkflowCommand* cause, const std::string& details) {
    WorkflowEvent e(type, this, workItemId, cause, details);
    notifyObservers(e);
}

void WorkflowInstance::notifyObservers(const WorkflowEvent& event) {
    WorkflowEvent e = event;
    if (!e.source) e.source = this;
    if (e.instanceId.empty()) e.instanceId = id;
    if (e.commandName.empty() && e.cause) e.commandName = e.cause->name();
    WorkflowSubject::notifyObservers(e);
}

void WorkflowInstance::accept(WorkflowVisitor& v) {
    v.visitInstance(*this);
    if (root) {
        root->accept(v);
    }
}

InstanceMemento* WorkflowInstance::createMemento() const {
    InstanceMemento* m = new InstanceMemento(data, std::string(), version);
    // Capture the actual work-item execution state, not a placeholder string.
    if (root) {
        class SnapshotVisitor : public WorkflowVisitor {
        public:
            explicit SnapshotVisitor(std::vector<WorkItemMemento>& snapshots)
                : snapshots(snapshots) {}
            void visitInstance(WorkflowInstance&) override {}
            void visitStage(Stage&) override {}
            void visitWorkItem(WorkItem& item) override {
                WorkItemMemento snapshot;
                snapshot.id = item.getId();
                snapshot.stateName = item.getStateName();
                snapshot.data = item.getAllData();
                snapshot.outcome = item.getOutcome();
                snapshot.failureReason = item.getFailureReason();
                snapshots.push_back(snapshot);
            }
            std::string getReport() const override { return std::string(); }
        private:
            std::vector<WorkItemMemento>& snapshots;
        };
        SnapshotVisitor visitor(m->workItems);
        root->accept(visitor);
    }
    // Capture stage activation/cursor state by structural path (0, 0/1, ...).
    // Paths are stable for this workflow definition and avoid assuming stage names are unique.
    class StageCapture {
    public:
        static void walk(const WorkflowComponent* component, const std::string& path,
                         std::vector<StageMemento>& snapshots) {
            const Stage* stage = component->asStage();
            if (!stage) return;
            Stage::RuntimeSnapshot runtime = stage->captureRuntimeSnapshot();
            StageMemento snapshot;
            snapshot.path = path; snapshot.activated = runtime.activated;
            snapshot.cancelledFlag = runtime.cancelledFlag;
            snapshot.finishedNotified = runtime.finishedNotified;
            snapshot.cursor = runtime.cursor; snapshot.cursorActivated = runtime.cursorActivated;
            snapshots.push_back(snapshot);
            for (std::size_t i = 0; i < stage->childCount(); ++i) {
                std::ostringstream childPath; childPath << path << "/" << i;
                walk(stage->childAt(i), childPath.str(), snapshots);
            }
        }
    };
    if (root) StageCapture::walk(root, "0", m->stages);

    // Keep a readable summary for diagnostics and audit tooling.
    for (std::size_t i = 0; i < m->workItems.size(); ++i) {
        if (i) m->stateSnapshot += ";";
        m->stateSnapshot += m->workItems[i].id + "=" + m->workItems[i].stateName;
    }
    return m;
}

void WorkflowInstance::restoreFromMemento(const InstanceMemento& memento) {
    // Restore instance-level values first.
    data = memento.data;
    version = memento.version;

    // Restore by stable work-item id; fail loudly if the saved workflow no longer
    // matches this instance rather than silently performing a partial rollback.
    std::vector<bool> restored(memento.workItems.size(), false);
    if (root) {
        class RestoreVisitor : public WorkflowVisitor {
        public:
            RestoreVisitor(const std::vector<WorkItemMemento>& snapshots,
                           std::vector<bool>& restored)
                : snapshots(snapshots), restored(restored) {}
            void visitInstance(WorkflowInstance&) override {}
            void visitStage(Stage&) override {}
            void visitWorkItem(WorkItem& item) override {
                for (std::size_t i = 0; i < snapshots.size(); ++i) {
                    if (snapshots[i].id == item.getId()) {
                        item.restoreSnapshot(snapshots[i].stateName, snapshots[i].data,
                                             snapshots[i].outcome, snapshots[i].failureReason);
                        restored[i] = true;
                        return;
                    }
                }
                throw std::runtime_error("Memento does not contain work item: " + item.getId());
            }
            std::string getReport() const override { return std::string(); }
        private:
            const std::vector<WorkItemMemento>& snapshots;
            std::vector<bool>& restored;
        };
        RestoreVisitor visitor(memento.workItems, restored);
        root->accept(visitor);
    }
    for (std::size_t i = 0; i < restored.size(); ++i) {
        if (!restored[i]) throw std::runtime_error("Memento references a missing work item: " + memento.workItems[i].id);
    }
    // Restore stage execution cursors after item states so sequential stages resume
    // from the same child instead of continuing from a later cursor.
    class StageRestore {
    public:
        static void walk(WorkflowComponent* component, const std::string& path,
                         const std::vector<StageMemento>& snapshots,
                         std::vector<bool>& restored) {
            Stage* stage = component->asStage();
            if (!stage) return;
            for (std::size_t i = 0; i < snapshots.size(); ++i) {
                if (snapshots[i].path == path) {
                    Stage::RuntimeSnapshot runtime;
                    runtime.activated = snapshots[i].activated;
                    runtime.cancelledFlag = snapshots[i].cancelledFlag;
                    runtime.finishedNotified = snapshots[i].finishedNotified;
                    runtime.cursor = snapshots[i].cursor;
                    runtime.cursorActivated = snapshots[i].cursorActivated;
                    stage->restoreRuntimeSnapshot(runtime);
                    restored[i] = true;
                    break;
                }
            }
            for (std::size_t i = 0; i < stage->childCount(); ++i) {
                std::ostringstream childPath; childPath << path << "/" << i;
                walk(stage->childAt(i), childPath.str(), snapshots, restored);
            }
        }
    };
    std::vector<bool> stagesRestored(memento.stages.size(), false);
    if (root) StageRestore::walk(root, "0", memento.stages, stagesRestored);
    for (std::size_t i = 0; i < stagesRestored.size(); ++i) {
        if (!stagesRestored[i]) throw std::runtime_error("Memento references a missing stage path: " + memento.stages[i].path);
    }
    if (coordinator) {
        coordinator->synchronizeAfterRestore();
        // A restored item gets its person back (the snapshot stores the participant id).
        const std::vector<WorkItem*>& all = coordinator->getItems();
        for (std::size_t i = 0; i < all.size(); ++i) {
            WorkItem* it = all[i];
            if (it->getDisposition() != WorkItemState::OPEN || it->isPending()) continue;   // only items someone was working on
            std::string who = it->getData("_assignee");
            for (std::size_t p = 0; p < participants.size(); ++p)
                if (participants[p]->getId() == who) { it->adoptAssignee(participants[p]); break; }
        }
    }
}

