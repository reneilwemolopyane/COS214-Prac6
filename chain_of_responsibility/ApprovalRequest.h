#pragma once
#include <string>

/** @file ApprovalRequest.h
 *  @brief Request travelling along the approval chain (Request role).
 *  `amount` is whatever value the handlers' limits are compared against. */
struct ApprovalRequest {
    double amount;              ///< value compared against each level's limit
    std::string workItemId;     ///< work item the approval belongs to
    std::string requester;      ///< who asked for the approval
    std::string deadline;       ///< deadline description
    std::string approvedBy;     ///< name of the level that approved
    std::string reason;         ///< reason recorded on rejection
    bool deadlinePassed;        ///< set by the engine's timer on no response

    ApprovalRequest() : amount(0), deadlinePassed(false) {}
};