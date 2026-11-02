#pragma once
#include <stdexcept>
#include <string>

/**
 * @brief Exception raised when an operation is not allowed in a work item's state.
 */
class InvalidTransitionException : public std::logic_error {
public:
    /**
     * @brief Creates a diagnostic for a disallowed state transition.
     * @param operation Requested operation.
     * @param stateName Current state name.
     * @param itemName Optional work-item name for the diagnostic.
     */
    InvalidTransitionException(const std::string& operation,
                                const std::string& stateName,
                                const std::string& itemName = "")
        : std::logic_error(buildMessage(operation, stateName, itemName)),
          operation_(operation), stateName_(stateName) {}

    /** @return The operation that was rejected. */
    const std::string& operation() const { return operation_; }
    /** @return The state in which the operation was attempted. */
    const std::string& stateName() const { return stateName_; }

private:
    static std::string buildMessage(const std::string& op,
                                    const std::string& state,
                                    const std::string& item) {
        std::string msg = "Cannot " + op + " work item";
        if (!item.empty()) msg += " '" + item + "'";
        msg += " in state " + state;
        return msg;
    }
    std::string operation_;
    std::string stateName_;
};