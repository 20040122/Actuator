#ifndef EXECUTOR_STATE_MANAGER_H
#define EXECUTOR_STATE_MANAGER_H

#include <map>
#include <string>
#include "../core/types.h"

class CommandStateManager {
public:
    void updateState(const std::string& node_id, NodeState state);
    NodeState getState(const std::string& node_id);
    void logTransition(const std::string& node_id, 
                       NodeState from, NodeState to);
    void reset();  // 清空所有节点状态，用于新任务
    
private:
    std::map<std::string, NodeState> states_;
};

#endif