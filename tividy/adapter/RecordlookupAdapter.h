#ifndef RECORDLOOKUPADAPTER_H
#define RECORDLOOKUPADAPTER_H
#include <string>
#include "tividy/adapter/ExternalConnector.h"
#include "tividy/adapter/RecordlookupAPI.h"

/** @brief Generic ADAPTER for any record/registry service (any industry).
 *  Used for "is this record valid?" checks and for fetching a stored value,
 *  e.g. policy cover, a police case number, a school results record, an identity check.
 *  Operations (request.operation), both take params: registry, key
 *   "verify"  -> data["valid"] = "true" or "false"; data["value"] when the record exists.
 *                A missing or inactive record is a normal answer (valid = "false"), not a failure.
 *   "lookup"  -> data["value"], data["active"]. A missing record is a permanent failure.
 *  Retryable failure = the service is down. Everything else is a permanent failure.
 *  A malformed request is reported as permanent even during an outage, because retrying it never helps.
 *  The adapter does not own the RecordLookupAPI it wraps. */
class RecordLookupAdapter : public ExternalConnector {
public:
    RecordLookupAdapter(const std::string& name, RecordLookupAPI* api) : n(name), adaptee(api) {}
    std::string name() const override { return n; }
    /** @brief Translates a generic request into RecordLookupAPI calls and the reply into a ConnectorResult. */
    ConnectorResult execute(const ConnectorRequest& request) override;
private:
    ConnectorResult verify(const std::string& registry, const std::string& key);
    ConnectorResult lookup(const std::string& registry, const std::string& key);

    std::string n; RecordLookupAPI* adaptee;
};

#endif