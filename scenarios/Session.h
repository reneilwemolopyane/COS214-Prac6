#ifndef SESSION_H
#define SESSION_H
/**
 * @file Session.h
 * @brief Everything a user interface needs: one Workspace per industry plus the checkpoints (Memento
 *        caretakers) used for "undo last step". Shared by the TUI and the web API so both behave the same.
 */
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "ScenarioCatalog.h"
#include "Workspace.h"
#include "tividy/memento/ProcessDataStore.h"

class Session {
public:
    Session() {
        std::vector<std::string> ids = ScenarioCatalog::ids();
        for (std::size_t i = 0; i < ids.size(); ++i) {
            order.push_back(ids[i]);
            workspaces[ids[i]].reset(new Workspace(ScenarioCatalog::create(ids[i])));
        }
    }
    const std::vector<std::string>& ids() const { return order; }
    /** @return the workspace for a scenario id, or null. */
    Workspace* find(const std::string& scenarioId) const {
        std::map<std::string, std::unique_ptr<Workspace> >::const_iterator it = workspaces.find(scenarioId);
        return it == workspaces.end() ? 0 : it->second.get();
    }
    /** @return the checkpoint store for an instance (created on first use). */
    ProcessDataStore& storeFor(const std::string& instanceId) {
        std::unique_ptr<ProcessDataStore>& s = checkpoints[instanceId];
        if (!s) s.reset(new ProcessDataStore());
        return *s;
    }
private:
    std::vector<std::string> order;
    std::map<std::string, std::unique_ptr<Workspace> > workspaces;
    std::map<std::string, std::unique_ptr<ProcessDataStore> > checkpoints;
    Session(const Session&); Session& operator=(const Session&);
};
#endif
