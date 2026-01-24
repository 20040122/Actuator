#include <iostream>
#include <thread>
#include <chrono>
#include "coordinator/multi_sat_coordinator.h"
#include "coordinator/inter_sat_comm.h"

using namespace coordinator;

void printSeparator(const std::string& title = "") {
    std::cout << "\n========================================";
    if (!title.empty()) {
        std::cout << "\n" << title;
    }
    std::cout << "\n========================================\n";
}

void printNodeList(const std::vector<NodeInfo>& nodes, const std::string& category) {
    std::cout << "\n" << category << " (" << nodes.size() << " 个):\n";
    for (const auto& node : nodes) {
        std::cout << "  - " << node.node_id 
                  << " [" << node.node_name << "]"
                  << " Type: " << node.node_type
                  << " Status: " << static_cast<int>(node.status);
        
        auto it = node.metadata.find("registration_type");
        if (it != node.metadata.end()) {
            std::cout << " (预注册)";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "多卫星协调器预注册节点示例\n";
    printSeparator();
    CoordinatorConfig config = CoordinatorConfig::getDefault();
    config.coordinator_id = "COORD_MAIN";
    config.coordinator_name = "主协调器";
    config.enable_auto_recovery = true;
    MultiSatCoordinator coordinator(config);
    printSeparator("初始化协调器");
    if (!coordinator.initialize()) {
        std::cerr << "协调器初始化失败！\n";
        return 1;
    }
    std::cout << "协调器初始化成功\n";
    printSeparator("加载全局配置");
    std::string global_config_file = "input/global.json";
    if (!coordinator.loadGlobalConfig(global_config_file)) {
        std::cerr << "全局配置加载失败！\n";
        return 1;
    }
    const auto& global_config = coordinator.getGlobalConfig();
    std::cout << "\n全局配置摘要:\n";
    std::cout << "  Plan ID: " << global_config.plan_id << "\n";
    std::cout << "  总节点数: " << global_config.total_nodes << "\n";
    std::cout << "  活跃节点: " << global_config.active_nodes.size() << " 个\n";
    for (const auto& node : global_config.active_nodes) {
        std::cout << "    - " << node << "\n";
    }
    
    // 加载调度计划
    printSeparator("加载调度计划");
    std::string schedule_file = "input/schedule.json";
    if (!coordinator.loadSchedule(schedule_file)) {
        std::cerr << "调度计划加载失败！\n";
        return 1;
    }
    
    // 显示已加载的调度信息
    const auto& schedule = coordinator.getSchedule();
    std::cout << "\n调度计划摘要:\n";
    std::cout << "  Plan ID: " << schedule.plan_id << "\n";
    std::cout << "  卫星数量: " << schedule.satellite_ids.size() << " 个\n";
    for (const auto& sat_id : schedule.satellite_ids) {
        std::cout << "    - " << sat_id;
        auto it = schedule.satellite_tasks.find(sat_id);
        if (it != schedule.satellite_tasks.end()) {
            std::cout << " (任务数: " << it->second.size() << ")";
        }
        std::cout << "\n";
    }
    
    // 等待预注册完成
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 检查预注册的节点
    printSeparator("检查预注册节点");
    auto node_registry = coordinator.getNodeRegistry();
    if (node_registry) {
        auto all_node_ids = node_registry->getAllNodeIds();
        std::cout << "节点注册表中共有 " << all_node_ids.size() << " 个节点\n";
        
        for (const auto& node_id : all_node_ids) {
            NodeInfo info;
            if (node_registry->getNodeInfo(node_id, info)) {
                std::cout << "\n节点: " << info.node_id << "\n";
                std::cout << "  名称: " << info.node_name << "\n";
                std::cout << "  类型: " << info.node_type << "\n";
                std::cout << "  状态: " << static_cast<int>(info.status);
                
                switch (info.status) {
                    case NodeStatus::OFFLINE: std::cout << " (离线)"; break;
                    case NodeStatus::HEALTHY: std::cout << " (健康)"; break;
                    case NodeStatus::DEGRADED: std::cout << " (降级)"; break;
                    case NodeStatus::FAULT: std::cout << " (故障)"; break;
                    default: std::cout << " (未知)"; break;
                }
                std::cout << "\n";
                
                std::cout << "  在线: " << (info.is_online ? "是" : "否") << "\n";
                std::cout << "  能力数量: " << info.capabilities.size() << "\n";
                
                for (const auto& cap : info.capabilities) {
                    std::cout << "    - " << cap.capability_id 
                              << ": " << cap.description << "\n";
                }
                
                if (!info.metadata.empty()) {
                    std::cout << "  元数据:\n";
                    for (const auto& meta : info.metadata) {
                        std::cout << "    " << meta.first << " = " << meta.second << "\n";
                    }
                }
            }
        }
        
        // 统计各类节点
        printSeparator("节点状态统计");
        auto healthy_nodes = coordinator.getHealthyNodes();
        auto degraded_nodes = coordinator.getDegradedNodes();
        auto offline_nodes = coordinator.getOfflineNodes();
        
        printNodeList(healthy_nodes, "健康节点");
        printNodeList(degraded_nodes, "降级节点");
        printNodeList(offline_nodes, "离线节点");
        
        // 显示注册表统计信息
        auto stats = node_registry->getStatistics();
        std::cout << "\n注册表统计:\n";
        std::cout << "  总节点数: " << stats.total_nodes << "\n";
        std::cout << "  在线节点: " << stats.online_nodes << "\n";
        std::cout << "  离线节点: " << stats.offline_nodes << "\n";
        std::cout << "  健康节点: " << stats.healthy_nodes << "\n";
        std::cout << "  降级节点: " << stats.degraded_nodes << "\n";
        std::cout << "  故障节点: " << stats.faulty_nodes << "\n";
    }
    
    // 手动调用预注册测试
    printSeparator("手动测试预注册");
    std::cout << "再次调用预注册函数（应跳过已存在的节点）...\n";
    coordinator.preRegisterNodes();
    
    // 启动协调器
    printSeparator("启动协调器");
    if (!coordinator.start()) {
        std::cerr << "协调器启动失败！\n";
        return 1;
    }
    std::cout << "协调器已启动，正在运行...\n";
    
    // 运行一段时间
    std::cout << "\n运行 5 秒...\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // 停止协调器
    printSeparator("停止协调器");
    coordinator.stop();
    std::cout << "协调器已停止\n";
    
    // 关闭协调器
    coordinator.shutdown();
    std::cout << "协调器已关闭\n";
    
    printSeparator("程序结束");
    std::cout << "预注册节点功能测试完成！\n\n";
    
    return 0;
}
