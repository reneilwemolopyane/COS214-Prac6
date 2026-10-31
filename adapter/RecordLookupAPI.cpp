#include "RecordLookupAPI.h"

namespace {
std::string idOf(const std::string& registry, const std::string& key) { return registry + "|" + key; }
}

void RecordLookupAPI::addRecord(const std::string& registry, const std::string& key,
                                const std::string& value, bool active) {
    Record r; r.value = value; r.active = active;
    records[idOf(registry, key)] = r;
}

bool RecordLookupAPI::find(const std::string& registry, const std::string& key,
                           std::string& value, bool& active) {
    if (down) return false;
    std::map<std::string, Record>::const_iterator it = records.find(idOf(registry, key));
    if (it == records.end()) return false;
    value = it->second.value;
    active = it->second.active;
    return true;
}