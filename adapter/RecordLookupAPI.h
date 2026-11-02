#ifndef RECORDLOOKUPAPI_H
#define RECORDLOOKUPAPI_H

#include <map>
#include <string>

/** @brief Typical record/registry service (ADAPTEE), simulated in memory.
 *  Holds records grouped by registry name (e.g. a policy register, a school results register).
 *  A record has a value and an "active" flag (e.g. a lapsed policy is not active).
 *  setDown(true) simulates an outage. */
class RecordLookupAPI {
public:
    RecordLookupAPI() : down(false) {}
    virtual ~RecordLookupAPI() {}

    /** @brief Adds or replaces a record. Used to prepare test and demo data. */
    void addRecord(const std::string& registry, const std::string& key,
                   const std::string& value, bool active = true);

    /** @return true if the record exists and the service is up. Fills value and active. */
    virtual bool find(const std::string& registry, const std::string& key,
                      std::string& value, bool& active);

    /** @return true if the service is up. */
    bool available() const { return !down; }
    /** @brief Simulates an outage (true) or recovery (false). */
    void setDown(bool d) { down = d; }

protected:
    struct Record { std::string value; bool active; };
    bool down;
    std::map<std::string, Record> records;   ///< "registry|key" -> record
};

#endif