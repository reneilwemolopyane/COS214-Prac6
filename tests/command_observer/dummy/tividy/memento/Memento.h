#pragma once
#include <string>
#include "tividy/core/Common.h"
struct InstanceMemento {
    DataMap data;
    std::string stateSnapshot;
    int version = 0;
};
