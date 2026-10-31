#pragma once
/**
 * @file ApprovalRouter.h
 * @brief Builds an approval chain from configuration and uses it to route a request.
 *
 * Client of the Chain of Responsibility: the router owns the LevelApprover objects,
 * links them in order and sends an ApprovalRequest into the chain. Which level answers
 * is decided by the chain (limits), never by if/else in the engine. The levels come from
 * the workflow definition as text, e.g. "Claims Handler=20000;Claims Manager=100000;Senior Manager=*",
 * so an organisation can change levels and limits without touching engine code.
 */
#include <string>
#include <vector>
#include "tividy/chain_of_responsibility/Approval.h"

class ApprovalRouter {
public:
    /** @throws std::invalid_argument if the text has no levels or a limit is not a number or '*'. */
    static ApprovalRouter* parse(const std::string& levels);
    ~ApprovalRouter();

    std::size_t levelCount() const { return handlers.size(); }
    const std::string& levelName(std::size_t i) const { return names[i]; }
    /** @return index of the level with this name, or -1. */
    int indexOf(const std::string& name) const;

    /**
     * @brief Finds the level that must approve @p amount.
     * @param amount value compared with each level's limit.
     * @param fromLevel -1 for a fresh request. Otherwise that level did not answer in time
     *        (its deadline passed), so the request moves to the next level that can approve.
     * @param note receives a readable line when the request moved up a level.
     * @return the responsible level index, or -1 when nobody in the chain can decide
     *         (end of chain: the engine keeps the request at the top level).
     */
    int route(double amount, int fromLevel, std::string& note);

private:
    ApprovalRouter() {}
    ApprovalRouter(const ApprovalRouter&); ApprovalRouter& operator=(const ApprovalRouter&);
    std::vector<ApprovalHandler*> handlers;   ///< Owned, linked head -> tail.
    std::vector<std::string> names;
};
