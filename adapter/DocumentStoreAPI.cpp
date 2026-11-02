#include "DocumentStoreAPI.h"
#include <sstream>

std::string DocumentStoreAPI::upload(const std::string& fileName, const std::string& contentType, const std::string& content) {
    if (down) return "";
    std::ostringstream o; o << "DOC-" << ++counter;
    Doc d; d.name = fileName; d.type = contentType; d.content = content;
    files[o.str()] = d;
    return o.str();
}
bool DocumentStoreAPI::download(const std::string& docId, std::string& content) {
    std::map<std::string, Doc>::const_iterator it = files.find(docId);
    if (down || it == files.end()) return false;
    content = it->second.content;
    return true;
}
bool DocumentStoreAPI::remove(const std::string& docId) {
    return !down && files.erase(docId) > 0;
}
