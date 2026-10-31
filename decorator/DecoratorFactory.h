#pragma once
#include <functional>
#include <string>
#include "tividy/composite_prototype/WorkflowComponent.h"

/**
 * @file DecoratorFactory.h
 * @brief Wraps a work item in the decorators a workflow definition asks for.
 *
 * A definition lists the extra behaviour of an item as text, e.g. "audit,validation:claimAmount,security:Finance Clerk".
 * The first name is wrapped first (innermost), so list `security` last to check the actor before anything else runs.
 * Supported names: audit, validation:<dataKey>, security:<Role>, priority, notification, deadline:<seconds>.
 */
class DecoratorFactory {
public:
    /** What the decorators need from the running instance. */
    struct Context {
        std::function<std::string()> actorRole;                          ///< Role of whoever is acting now.
        std::function<std::string(const std::string&)> data;             ///< Reads an instance data value.
        std::function<void(const std::string&)> audit;                   ///< Receives audit entries.
        std::function<void(const std::string&)> notify;                  ///< Receives notification texts.
    };

    /** @throws std::invalid_argument if the spec contains an unknown decorator or a missing argument. */
    static void validate(const std::string& spec);

    /** @brief Wraps @p item in the decorators named by @p spec; returns the outermost component. */
    static WorkflowComponent* wrap(WorkflowComponent* item, const std::string& spec, const Context& ctx);
};
