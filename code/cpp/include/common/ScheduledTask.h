#pragma once
#include <string>

namespace actuator {

struct ScheduledTask {
    std::string task_id;
    int window_index;
    std::string actual_start;
    std::string actual_end;
    int profit;
};

}
