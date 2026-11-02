#ifndef SCENARIO_H
#define SCENARIO_H
/**
 * @file Scenario.h
 * @brief One industry that runs on the generic TIVIDY engine.
 *
 * Template Method: install() fixes the order (build the definition, add the people, add the
 * external systems); each industry fills in only the variable steps. Nothing here changes the
 * engine: a scenario is configuration plus stub adapters.
 */
#include <string>
#include <vector>
#include "tividy/core/Common.h"
#include "tividy/builder/WorkflowDirector.h"
#include "tividy/facade/DefinitionRepository.h"
#include "tividy/command/WorkflowEngine.h"

/** @brief A ready-made input a marker can start with one keystroke. */
struct SampleCase {
    std::string key;      ///< short id, e.g. "repair-small"
    std::string label;    ///< one-line title
    std::string story;    ///< what the case demonstrates
    DataMap data;         ///< the instance data
};

class Scenario {
public:
    virtual ~Scenario() {}

    const std::string& id() const { return scenarioId; }             ///< definition id, e.g. "motor-claim"
    const std::string& industry() const { return industryName; }     ///< e.g. "Insurance"
    const std::string& title() const { return scenarioTitle; }
    const std::string& summary() const { return scenarioSummary; }

    /** @brief Template Method: builds the definition, adds participants, registers the connectors. */
    void install(WorkflowEngine& engine, DefinitionRepository& repo);

    virtual std::vector<SampleCase> sampleCases() const = 0;
    /** @brief Names of the stub external systems that can be switched off to show retries and incidents. */
    virtual std::vector<std::string> systems() const = 0;
    /** @brief Simulates an outage (true) or recovery (false). @return false for an unknown system. */
    virtual bool setOutage(const std::string& system, bool down) = 0;

protected:
    Scenario(const std::string& id, const std::string& industry, const std::string& title, const std::string& summary)
        : scenarioId(id), industryName(industry), scenarioTitle(title), scenarioSummary(summary) {}

    virtual WorkflowDirector& director() = 0;                          ///< step 1 (varies)
    virtual void addParticipants(WorkflowEngine& engine) = 0;          ///< step 2 (varies)
    virtual void addConnectors(ConnectorRegistry& registry) = 0;       ///< step 3 (varies)

private:
    std::string scenarioId, industryName, scenarioTitle, scenarioSummary;
};

#endif
