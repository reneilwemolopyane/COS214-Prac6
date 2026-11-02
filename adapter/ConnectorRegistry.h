#ifndef CONNECTORREGISTRY_H
#define CONNECTORREGISTRY_H
/**
 * @file ConnectorRegistry.h
 * @brief Looks connectors up by configured name so the engine never names a specific system.
 */
#include <map>
#include <string>
#include "ExternalConnector.h"

/** @brief Name-to-connector lookup. Does not own the connectors. */
class ConnectorRegistry {
public:
    /** @brief Registers (or replaces) a connector under key. */
    void add(const std::string& key, ExternalConnector* connector) { connectors[key] = connector; }
    /** @return the connector registered under key, or 0 if the key is unknown. */
    ExternalConnector* find(const std::string& key) const {
        std::map<std::string, ExternalConnector*>::const_iterator it = connectors.find(key);
        return it == connectors.end() ? 0 : it->second;
    }
    /** @brief Runs request on the named connector. An unknown name gives a permanent failed result, never a null dereference. */
    ConnectorResult execute(const std::string& key, const ConnectorRequest& request) const {
        ExternalConnector* c = find(key);
        if (!c) { ConnectorResult r; r.message = "no connector registered as '" + key + "'"; return r; }
        return c->execute(request);
    }
private:
    std::map<std::string, ExternalConnector*> connectors;
};
#endif
