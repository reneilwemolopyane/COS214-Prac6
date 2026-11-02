#ifndef EXTERNALCONNECTOR_H
#define EXTERNALCONNECTOR_H
/**
 * @file ExternalConnector.h
 * @brief Adapter pattern: Target interface.
 */
#include "ConnectorResult.h"


/**
 * @brief Target: the only external-system interface the engine knows.
 * 
 */
class ExternalConnector {
public:
    virtual ~ExternalConnector() {}
    /** @return the configured name of this connector. */
    virtual std::string name() const = 0;
    /** @brief Performs a generic request and returns a common result (never throws for external failures). */
    virtual ConnectorResult execute(const ConnectorRequest& request) = 0;
};
#endif
