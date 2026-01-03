#pragma once
#include <string>
#include <map>
#include "PlannerInput.h"
#include "../behavior_tree/BehaviorTreeNode.h"

namespace actuator {

struct ExecutorInput {
    std::string project_name;
    PlannerInput planner_input;
    bt::BehaviorDefinitions behavior_definitions;
};

}
