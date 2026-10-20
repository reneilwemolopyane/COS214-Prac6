#pragma once
/** @file ConnectorRegistry.h  Looks connectors up by configured name. STUB */
#include <map>
#include "tividy/adapter/ExternalConnector.h"
class ConnectorRegistry {
public:
    void add(const std::string&, ExternalConnector*) {}       // TODO
    ExternalConnector* find(const std::string&) const { return 0; }  // TODO
private: std::map<std::string, ExternalConnector*> connectors;
};
