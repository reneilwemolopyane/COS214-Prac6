#pragma once
/** @file ExternalConnector.h  Adapter TARGET + ConcreteColleague of Mediator. (ADAPTER OWNER) */
#include "tividy/adapter/ConnectorResult.h"
#include "tividy/mediator/Mediator.h"
class ExternalConnector : public WorkflowColleague {
public:
    virtual ~ExternalConnector() {}
    virtual std::string name() const = 0;
    virtual ConnectorResult execute(const ConnectorRequest& request) = 0;
};
