#pragma once
#include <string>
#include <vector>
#include <map>
#include "TimeWindow.h"
#include "Task.h"
#include "ScheduledTask.h"

namespace actuator {

struct PlannerInput {
    std::vector<std::string> satellites;
    std::vector<Task> tasks;
    std::map<std::string, std::map<std::string, std::vector<TimeWindow>>> windows;
    std::map<std::string, std::vector<ScheduledTask>> scheduled_tasks;
    std::vector<std::string> unscheduled_tasks;
    int total_profit;
    int maneuver_time;
};

}
