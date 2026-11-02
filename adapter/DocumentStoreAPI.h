#ifndef DOCUMENTSTOREAPI_H
#define DOCUMENTSTOREAPI_H

#include <map>
#include <string>
/** @brief Typical document/file store (ADAPTEE). Uploads, downloads and deletes documents.
 *  Simulated in memory for tests; setDown(true) simulates an outage. */
class DocumentStoreAPI {
public:
    DocumentStoreAPI() : down(false), counter(0) {}
    virtual ~DocumentStoreAPI() {}
    /** @return a document id, or "" on failure (service down). */
    virtual std::string upload(const std::string& fileName, const std::string& contentType, const std::string& content);
    /** @return false if the document does not exist or the service is down. */
    virtual bool download(const std::string& docId, std::string& content);
    virtual bool remove(const std::string& docId);
    bool available() const { return !down; }
    bool exists(const std::string& docId) const { return files.count(docId) > 0; }
    void setDown(bool d) { down = d; }
protected:
    struct Doc { std::string name, type, content; };
    bool down;
    int counter;
    std::map<std::string, Doc> files;   // docId -> document
};

#endif
