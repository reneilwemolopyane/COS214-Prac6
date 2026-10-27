#pragma once
#include <stdexcept>
#include <string>

class InvalidTransitionException : public std::logic_error {
public:
    InvalidTransitionException(const std::string& operation,
                                const std::string& stateName,
                                const std::string& itemName = "")
        : std::logic_error(buildMessage(operation, stateName, itemName)),
          operation_(operation), stateName_(stateName) {}

    const std::string& operation() const { return operation_; }
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