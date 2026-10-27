#ifndef DOCUMENTSTORAGEADAPTER_H
#define DOCUMENTSTORAGEADAPTER_H
#include <string>
#include "ExternalConnector.h"
#include "DocumentStoreAPI.h"

/** @brief Generic ADAPTER for any document/file store (any industry).
 *  Operations (request.operation):
 *   "store"   params: name, contentType, content       -> data["documentId"]
 *   "fetch"   params: documentId                       -> data["content"]
 *   "delete"  params: documentId
 *  Retryable failure = the store is down. Everything else is a permanent failure. */
class DocumentStorageAdapter : public ExternalConnector {
public:
    DocumentStorageAdapter(const std::string& name, DocumentStoreAPI* api) : n(name), adaptee(api) {}
    std::string name() const override{ return n; }
    /** @brief Translates a generic request into adaptee calls and the reply into a ConnectorResult. */
    ConnectorResult execute(const ConnectorRequest& request)override;
private:
    ConnectorResult storeDocument(const ConnectorRequest& request);
    ConnectorResult fetchDocument(const ConnectorRequest& request);
    ConnectorResult deleteDocument(const ConnectorRequest& request);

    std::string n; DocumentStoreAPI* adaptee;
};

#endif
