#include <iostream>
#include "input/json_parser.h"
#include "executor/simple_executor.cpp"

int main() {
    std::cout << "=== 单星测试 ===" << std::endl;
    ScheduleParser schedule_parser;
    auto tasks = schedule_parser.parseSatelliteTasks(
        "input/schedule.json", "S1"
    );
    if (tasks.empty()) {
        std::cerr << "未找到S1的任务" << std::endl;
        return 1;
    }
    BehaviorLibraryParser lib_parser;
    SimpleExecutor executor;
    std::cout << "\n加载全局配置..." << std::endl;
    executor.getVariableManager().loadFromGlobalConfig("input/global.json");
    std::cout << "加载调度配置..." << std::endl;
    executor.getVariableManager().loadFromScheduleConfig("input/schedule.json", "S1");
    std::cout << "配置加载完成\n" << std::endl;
    
    for (size_t i = 0; i < tasks.size(); ++i) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "执行 " << (i + 1) << "/" << tasks.size() << std::endl;
        std::cout << "任务ID: " << tasks[i].task_id << std::endl;
        std::cout << "段ID: " << tasks[i].segment_id << std::endl;
        std::cout << "行为: " << tasks[i].behavior_ref << std::endl;
        std::cout << "========================================\n" << std::endl;
        auto behavior_def = lib_parser.parseBehaviorDefinition(
            "input/behaviorTree.json",
            tasks[i].behavior_ref 
        );
        executor.executeTask(tasks[i], behavior_def);
    }
    std::cout << "\n========================================" << std::endl;
    std::cout << "执行完成，共执行 " << tasks.size() << " 个任务" << std::endl;
    return 0;
}