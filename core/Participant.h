#pragma once
#include <string>
/** @brief A person/role that performs work. STUB */
class Participant {
public:
    Participant(const std::string& id = "", const std::string& name = "", const std::string& role = "")
        : id(id), name(name), role(role), workload(0) {}
    std::string id, name, role;
    int workload;
};
