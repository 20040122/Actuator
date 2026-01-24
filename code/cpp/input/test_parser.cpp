#include "json_parser.h"
#include <iostream>
#include <iomanip>
#include <cassert>

void printSeparator(const std::string& title) {
    std::cout << "\n========================================" << std::endl;
    std::cout << title << std::endl;
    std::cout << "========================================" << std::endl;
}

void testParseAllSatellites() {
    printSeparator("测试: 解析所有卫星调度");
    
    ScheduleParser parser;
    auto result = parser.parseAllSatellites("code/cpp/input/schedule.json");
    
    // 验证基本信息
    std::cout << "\n[基本信息]" << std::endl;
    std::cout << "  计划ID: " << result.plan_id << std::endl;
    std::cout << "  调度ID: " << result.schedule_id << std::endl;
    std::cout << "  卫星数量: " << result.satellite_ids.size() << std::endl;
    
    assert(!result.plan_id.empty() && "plan_id 不应为空");
    assert(result.satellite_ids.size() == 2 && "应该有2颗卫星");
    std::cout << "  ✓ 基本信息验证通过" << std::endl;
    
    // 验证每颗卫星的任务
    std::cout << "\n[卫星任务详情]" << std::endl;
    for (const auto& sat_id : result.satellite_ids) {
        std::cout << "\n  卫星: " << sat_id << std::endl;
        
        auto it = result.satellite_tasks.find(sat_id);
        assert(it != result.satellite_tasks.end() && "应该找到卫星任务");
        
        const auto& tasks = it->second;
        std::cout << "    任务数量: " << tasks.size() << std::endl;
        
        for (size_t i = 0; i < tasks.size(); ++i) {
            const auto& task = tasks[i];
            std::cout << "\n    任务 #" << (i + 1) << ":" << std::endl;
            std::cout << "      - segment_id: " << task.segment_id << std::endl;
            std::cout << "      - task_id: " << task.task_id << std::endl;
            std::cout << "      - behavior_ref: " << task.behavior_ref << std::endl;
            std::cout << "      - 计划开始: " << task.execution.planned_start << std::endl;
            std::cout << "      - 计划结束: " << task.execution.planned_end << std::endl;
            std::cout << "      - 持续时间: " << task.execution.duration_s << "秒" << std::endl;
            std::cout << "      - 窗口ID: " << task.window.window_id << std::endl;
            
            // 验证必要字段
            assert(!task.segment_id.empty() && "segment_id 不应为空");
            assert(!task.task_id.empty() && "task_id 不应为空");
            assert(!task.behavior_ref.empty() && "behavior_ref 不应为空");
            assert(task.execution.duration_s > 0 && "duration 应该大于0");
            
            // 验证 behavior_params
            std::cout << "      - 行为参数:" << std::endl;
            for (const auto& param : task.behavior_params) {
                std::cout << "          " << param.first << ": " << param.second << std::endl;
            }
            assert(!task.behavior_params.empty() && "behavior_params 不应为空");
        }
    }
    
    // 验证具体卫星
    std::cout << "\n[具体验证]" << std::endl;
    
    // 验证 S1
    auto s1_tasks = result.satellite_tasks["S1"];
    assert(s1_tasks.size() == 3 && "S1应该有3个任务");
    assert(s1_tasks[0].task_id == "T1" && "S1第一个任务应该是T1");
    assert(s1_tasks[1].task_id == "T4" && "S1第二个任务应该是T4");
    assert(s1_tasks[2].task_id == "T500" && "S1第三个任务应该是T500");
    std::cout << "  ✓ S1 任务验证通过 (3个任务: T1, T4, T500)" << std::endl;
    
    // 验证 S2
    auto s2_tasks = result.satellite_tasks["S2"];
    assert(s2_tasks.size() == 3 && "S2应该有3个任务");
    assert(s2_tasks[0].task_id == "T2" && "S2第一个任务应该是T2");
    assert(s2_tasks[1].task_id == "T5" && "S2第二个任务应该是T5");
    assert(s2_tasks[2].task_id == "T9" && "S2第三个任务应该是T9");
    std::cout << "  ✓ S2 任务验证通过 (3个任务: T2, T5, T9)" << std::endl;
    
    // 验证特定任务的参数
    auto& t1 = s1_tasks[0];
    assert(t1.behavior_params.count("target_lat") > 0 && "应该有target_lat参数");
    assert(t1.behavior_params.count("target_lon") > 0 && "应该有target_lon参数");
    assert(t1.behavior_params.count("profit") > 0 && "应该有profit参数");
    assert(t1.behavior_params.count("resource_semaphore") > 0 && "应该有resource_semaphore参数");
    std::cout << "  ✓ 任务参数验证通过" << std::endl;
    
    std::cout << "\n✅ parseAllSatellites() 测试全部通过!" << std::endl;
}

void testGlobalConfigParser() {
    printSeparator("测试: 解析全局配置");
    
    GlobalConfigParser parser;
    auto config = parser.parse("code/cpp/input/global.json");
    
    // 验证基本信息
    std::cout << "\n[基本信息]" << std::endl;
    std::cout << "  计划ID: " << config.plan_id << std::endl;
    std::cout << "  总节点数: " << config.total_nodes << std::endl;
    std::cout << "  活跃节点数: " << config.active_nodes.size() << std::endl;
    
    assert(config.plan_id == "observation_plan_001" && "plan_id应该是observation_plan_001");
    assert(config.total_nodes == 128 && "总节点数应该是128");
    assert(config.active_nodes.size() == 2 && "活跃节点数应该是2");
    std::cout << "  ✓ 基本信息验证通过" << std::endl;
    
    // 验证活跃节点
    std::cout << "\n[活跃节点]" << std::endl;
    for (const auto& node : config.active_nodes) {
        std::cout << "  - " << node << std::endl;
    }
    assert(config.active_nodes[0] == "S1" && "第一个活跃节点应该是S1");
    assert(config.active_nodes[1] == "S2" && "第二个活跃节点应该是S2");
    std::cout << "  ✓ 活跃节点验证通过" << std::endl;
    
    // 验证通信配置
    std::cout << "\n[节点通信配置]" << std::endl;
    std::cout << "  协议: " << config.node_communication.protocol << std::endl;
    std::cout << "  最大延迟: " << config.node_communication.max_latency_ms << "ms" << std::endl;
    std::cout << "  重试次数: " << config.node_communication.retry_count << std::endl;
    
    assert(config.node_communication.protocol == "INTER_SAT_LINK" && "协议应该是INTER_SAT_LINK");
    assert(config.node_communication.max_latency_ms == 500 && "最大延迟应该是500ms");
    assert(config.node_communication.retry_count == 3 && "重试次数应该是3");
    std::cout << "  ✓ 通信配置验证通过" << std::endl;
    
    // 验证信号量配置
    std::cout << "\n[信号量配置]" << std::endl;
    std::cout << "  信号量数量: " << config.semaphores.size() << std::endl;
    assert(config.semaphores.size() == 4 && "应该有4个信号量");
    
    for (size_t i = 0; i < config.semaphores.size(); ++i) {
        const auto& sem = config.semaphores[i];
        std::cout << "\n  信号量 #" << (i + 1) << ":" << std::endl;
        std::cout << "    - ID: " << sem.semaphore_id << std::endl;
        std::cout << "    - 资源名称: " << sem.resource_name << std::endl;
        std::cout << "    - 资源类型: " << sem.resource_type << std::endl;
        std::cout << "    - 最大许可: " << sem.max_permits << std::endl;
        std::cout << "    - 可用许可: " << sem.available_permits << std::endl;
        std::cout << "    - 队列策略: " << sem.queue_policy << std::endl;
        std::cout << "    - 超时时间: " << sem.timeout_s << "秒" << std::endl;
        std::cout << "    - 优先级启用: " << (sem.priority_enabled ? "是" : "否") << std::endl;
        
        assert(!sem.semaphore_id.empty() && "semaphore_id不应为空");
        assert(!sem.resource_name.empty() && "resource_name不应为空");
        assert(sem.max_permits > 0 && "max_permits应该大于0");
    }
    
    // 验证特定信号量
    assert(config.semaphores[0].semaphore_id == "sem_ground_station_beijing" && 
           "第一个信号量应该是北京地面站");
    assert(config.semaphores[0].max_permits == 3 && "北京地面站最大许可应该是3");
    std::cout << "  ✓ 信号量配置验证通过" << std::endl;
    
    // 验证同步屏障
    std::cout << "\n[同步屏障配置]" << std::endl;
    std::cout << "  屏障数量: " << config.barriers.size() << std::endl;
    assert(config.barriers.size() == 2 && "应该有2个同步屏障");
    
    for (size_t i = 0; i < config.barriers.size(); ++i) {
        const auto& barrier = config.barriers[i];
        std::cout << "\n  屏障 #" << (i + 1) << ":" << std::endl;
        std::cout << "    - ID: " << barrier.sync_id << std::endl;
        std::cout << "    - 类型: " << barrier.type << std::endl;
        std::cout << "    - 锚点时间: " << barrier.anchor_time << std::endl;
        std::cout << "    - 时间窗口: " << barrier.window_s << "秒" << std::endl;
        std::cout << "    - 超时时间: " << barrier.timeout_s << "秒" << std::endl;
        std::cout << "    - 参与者数量: " << barrier.participants.size() << std::endl;
        
        for (const auto& participant : barrier.participants) {
            std::cout << "      * " << participant << std::endl;
        }
        
        assert(!barrier.sync_id.empty() && "sync_id不应为空");
        assert(!barrier.type.empty() && "type不应为空");
        assert(barrier.participants.size() > 0 && "应该有参与者");
    }
    
    assert(config.barriers[0].sync_id == "morning_sync" && "第一个屏障应该是morning_sync");
    assert(config.barriers[0].participants.size() == 2 && "morning_sync应该有2个参与者");
    std::cout << "  ✓ 同步屏障配置验证通过" << std::endl;
    
    // 验证资源分配策略
    std::cout << "\n[资源分配策略]" << std::endl;
    std::cout << "  死锁检测: " << (config.deadlock_detection ? "启用" : "禁用") << std::endl;
    std::cout << "  死锁解决策略: " << config.deadlock_resolution << std::endl;
    
    assert(config.deadlock_detection == true && "死锁检测应该启用");
    assert(config.deadlock_resolution == "ABORT_LOWER_PRIORITY" && 
           "死锁解决策略应该是ABORT_LOWER_PRIORITY");
    std::cout << "  ✓ 资源分配策略验证通过" << std::endl;
    
    std::cout << "\n✅ GlobalConfigParser::parse() 测试全部通过!" << std::endl;
}

void testIntegration() {
    printSeparator("测试: 集成验证");
    
    // 同时解析两个文件
    ScheduleParser schedule_parser;
    GlobalConfigParser config_parser;
    
    auto schedule = schedule_parser.parseAllSatellites("code/cpp/input/schedule.json");
    auto config = config_parser.parse("code/cpp/input/global.json");
    
    std::cout << "\n[一致性验证]" << std::endl;
    
    // 验证 plan_id 一致性
    assert(schedule.plan_id == config.plan_id && "两个文件的plan_id应该一致");
    std::cout << "  ✓ plan_id 一致 (" << schedule.plan_id << ")" << std::endl;
    
    // 验证卫星数量与活跃节点数量一致
    assert(schedule.satellite_ids.size() == config.active_nodes.size() && 
           "卫星数量应该与活跃节点数量一致");
    std::cout << "  ✓ 卫星/节点数量一致 (" << schedule.satellite_ids.size() << ")" << std::endl;
    
    // 验证卫星ID与活跃节点对应
    for (const auto& sat_id : schedule.satellite_ids) {
        bool found = false;
        for (const auto& node : config.active_nodes) {
            if (node == sat_id) {
                found = true;
                break;
            }
        }
        assert(found && "每个卫星ID都应该在活跃节点列表中");
    }
    std::cout << "  ✓ 卫星ID与活跃节点匹配" << std::endl;
    
    // 验证任务中使用的信号量在全局配置中定义
    std::cout << "\n[信号量引用验证]" << std::endl;
    int semaphore_refs = 0;
    for (const auto& sat_id : schedule.satellite_ids) {
        const auto& tasks = schedule.satellite_tasks[sat_id];
        for (const auto& task : tasks) {
            if (task.behavior_params.count("resource_semaphore") > 0) {
                const std::string& sem_id = task.behavior_params.at("resource_semaphore");
                
                // 检查这个信号量是否在全局配置中定义
                bool found = false;
                for (const auto& sem : config.semaphores) {
                    if (sem.semaphore_id == sem_id) {
                        found = true;
                        semaphore_refs++;
                        std::cout << "  ✓ 任务 " << task.task_id << " 引用的信号量 " 
                                  << sem_id << " 已定义" << std::endl;
                        break;
                    }
                }
                assert(found && "任务引用的信号量应该在全局配置中定义");
            }
        }
    }
    std::cout << "  共验证 " << semaphore_refs << " 个信号量引用" << std::endl;
    
    std::cout << "\n✅ 集成测试全部通过!" << std::endl;
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  多星任务解析器测试程序" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // 测试1: 解析所有卫星调度
        testParseAllSatellites();
        
        // 测试2: 解析全局配置
        testGlobalConfigParser();
        
        // 测试3: 集成验证
        testIntegration();
        
        // 总结
        printSeparator("测试总结");
        std::cout << "\n🎉 所有测试通过!" << std::endl;
        std::cout << "\n测试覆盖:" << std::endl;
        std::cout << "  ✓ 多卫星调度解析 (parseAllSatellites)" << std::endl;
        std::cout << "  ✓ 全局配置解析 (GlobalConfigParser::parse)" << std::endl;
        std::cout << "  ✓ 数据一致性验证" << std::endl;
        std::cout << "  ✓ 信号量引用验证" << std::endl;
        std::cout << "\n解析器新增代码功能正常! ✨" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 测试失败: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n❌ 测试失败: 未知错误" << std::endl;
        return 1;
    }
}
