#include <iostream>
#include "../core/types.h"
#include "../parser/behavior_parser.h"
#include "state_manager.h"

class SimpleExecutor {
public:
    void executeTask(const TaskSegment& task, 
                     const BehaviorNode& behavior_def) {
        BehaviorTreeParser parser;
        auto nodes = parser.instantiate(behavior_def, task.behavior_params);
        for (const auto& node : nodes) {
            std::cout << "执行节点: " << node.name << std::endl;
            state_mgr_.updateState(node.name, NodeState::RUNNING);
            bool success = executeNode(node);
            state_mgr_.updateState(node.name, 
                success ? NodeState::SUCCESS : NodeState::FAILED);
        }
    }
    
private:
    CommandStateManager state_mgr_;
    bool executeNode(const BehaviorNode& node) {
        std::cout << "  → 命令: " << node.command << std::endl;
        return true;  
    }
};