#pragma once
#include <string>

namespace actuator {

struct Task {
    std::string id;
    std::string name;
    int profit;
    int duration;
    std::string deadline;
};

}
