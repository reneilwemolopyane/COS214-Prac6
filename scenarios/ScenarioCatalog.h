#ifndef SCENARIOCATALOG_H
#define SCENARIOCATALOG_H
#include <string>
#include <vector>
#include "Scenario.h"

/** @brief Factory Method: the industries available to the user interfaces. */
class ScenarioCatalog {
public:
    /** @return ids of every available scenario, in menu order. */
    static std::vector<std::string> ids();
    /** @return a new scenario (caller owns it), or null for an unknown id. */
    static Scenario* create(const std::string& id);
};
#endif
