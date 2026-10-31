#pragma once
#include <string>
#include "ApprovalHandler.h"

/** @brief ConcreteHandler: one generic level. Name, limit and decision
 *  come from the workflow definition / main.cpp. */
class LevelApprover : public ApprovalHandler {
public:
    LevelApprover(const std::string& label, double maxAmount,
                  Decision d = Decision());
protected:
    std::string levelName() const override;
private:
    std::string name;
};