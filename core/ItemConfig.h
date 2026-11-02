#pragma once
/**
 * @file ItemConfig.h
 * @brief Everything a definition says about ONE work item beyond its name and role.
 *
 * It is plain configuration text. The generic engine reads it; only a scenario writes it.
 * That is how two industries use the same engine classes without modifying them.
 */
#include <string>

struct ItemConfig {
    std::string guard;            ///< Rule (Interpreter) that must hold for the item to run; else it is skipped.
    std::string decorations;      ///< Comma list: audit, validation:<dataKey>, security:<Role>, priority, notification, deadline:<seconds>.
    std::string approvalAmountKey;///< Instance-data key holding the amount the approval chain is compared against.
    std::string approvalLevels;   ///< "Role=limit;Role=limit;Role=*" (Chain of Responsibility, built by ApprovalRouter).
    std::string escalationRole;   ///< Role that takes over when no one responds (non-approval items).
    std::string assignmentRule;   ///< Strategy name: role, shortest-queue, round-robin.
    std::string serviceConnector; ///< Connector name in the registry (Adapter), called before the item completes.
    std::string serviceOperation; ///< Operation to invoke, e.g. "pay" or "verify".
    std::string serviceParams;    ///< "name=value;name=$dataKey;name=$dataKey*100".
    std::string serviceUndo;      ///< Operation used to reverse the call on rollback (default "reverse").
    long deadlineSeconds;         ///< How long an assigned item may wait for pickup before it is escalated (0 = none).
    ItemConfig() : deadlineSeconds(0) {}
};
