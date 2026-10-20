#pragma once
#include <string>
/** @brief Command receiver for external calls. STUB */
class ExternalService {
public:
    virtual ~ExternalService() {}
    virtual std::string call(const std::string&) { return ""; }   // TODO
    virtual bool reverse(const std::string&) { return false; }    // TODO
};
