#include "LevelApprover.h"

LevelApprover::LevelApprover(const std::string& label, double maxAmount, Decision d)
    : ApprovalHandler(maxAmount, d), name(label) {}

std::string LevelApprover::levelName() const { return name; }