#include "tividy/adapter/AdapterHelpers.h"
#include "tividy/adapter/DocumentStorageAdapter.h"

// Checks that the store is reachable, then hands the request to the right method.
ConnectorResult DocumentStorageAdapter::execute(const ConnectorRequest& request) {
    if (adaptee == 0) return failed(n, false, "no document store client");
    if (!adaptee->available()) return failed(n, true, "document store unavailable");

    if (request.operation == "store") return storeDocument(request);
    if (request.operation == "fetch") return fetchDocument(request);
    if (request.operation == "delete") return deleteDocument(request);
    return failed(n, false, "unknown operation '" + request.operation + "'");
}

// "store": saves a document and returns its id.
ConnectorResult DocumentStorageAdapter::storeDocument(const ConnectorRequest& request) {
    std::string name = param(request.params, "name");
    if (name.empty()) return failed(n, false, "missing document name");

    std::string content = param(request.params, "content");
    if (content.empty()) return failed(n, false, "empty document");

    std::string id = adaptee->upload(name, param(request.params, "contentType"), content);
    if (id.empty()) return failed(n, true, "upload failed, try again");

    ConnectorResult result(true, false, "stored");
    result.data["documentId"] = id;
    return result;
}

// "fetch": returns the content of a stored document.
ConnectorResult DocumentStorageAdapter::fetchDocument(const ConnectorRequest& request) {
    std::string id = param(request.params, "documentId");
    if (id.empty()) return failed(n, false, "missing documentId");

    std::string content;
    if (!adaptee->download(id, content)) return failed(n, false, "document not found");

    ConnectorResult result(true, false, "fetched");
    result.data["content"] = content;
    return result;
}

// "delete": removes a stored document.
ConnectorResult DocumentStorageAdapter::deleteDocument(const ConnectorRequest& request) {
    std::string id = param(request.params, "documentId");
    if (id.empty()) return failed(n, false, "missing documentId");

    if (!adaptee->remove(id)) return failed(n, false, "document not found");
    return ConnectorResult(true, false, "deleted");
}
