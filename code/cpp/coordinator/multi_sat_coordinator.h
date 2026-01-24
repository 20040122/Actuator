#ifndef COORDINATOR_MULTI_SAT_COORDINATOR_H
#define COORDINATOR_MULTI_SAT_COORDINATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <functional>

#include "inter_sat_comm.h"
#include "node_registry.h"
#include "message_types.h"
#include "../core/types.h"
#include "../input/json_parser.h"

namespace coordinator {

enum class TaskLifecycleState : uint8_t {
    CREATED,
    ASSIGNED,
    ACKNOWLEDGED,
    QUEUED,
    EXECUTING,
    COMPLETED,
    FAILED,
    CANCELLED,
    TIMEOUT
};

struct TaskRecord {
    std::string segment_id;
    std::string task_id;
    std::string assigned_node_id;
    TaskLifecycleState state;
    Priority priority;
    TaskAssignMessage task_message;
    uint64_t create_time_ms;
    uint64_t assign_time_ms;
    uint64_t start_time_ms;
    uint64_t complete_time_ms;
    uint64_t timeout_ms;
    uint8_t progress_percent;
    std::string current_action;
    std::string result_summary;
    bool success;
    int retry_count;
    int max_retries;
    
    TaskRecord() : state(TaskLifecycleState::CREATED),
                   priority(Priority::NORMAL),
                   create_time_ms(0), assign_time_ms(0),
                   start_time_ms(0), complete_time_ms(0),
                   timeout_ms(0), progress_percent(0),
                   success(false), retry_count(0), max_retries(3) {}
};

struct CoordinatorConfig {
    std::string coordinator_id;
    std::string coordinator_name;
    uint32_t main_loop_interval_ms;
    uint32_t health_check_interval_ms;
    uint32_t task_timeout_check_interval_ms;
    uint32_t task_default_timeout_ms;
    uint32_t node_health_timeout_ms;
    uint32_t max_tasks_per_node;
    uint32_t max_retry_attempts;
    bool enable_task_rebalancing;
    bool enable_auto_recovery;
    
    static CoordinatorConfig getDefault() {
        CoordinatorConfig config;
        config.coordinator_id = "COORDINATOR_MAIN";
        config.coordinator_name = "Main Coordinator";
        config.main_loop_interval_ms = 100;
        config.health_check_interval_ms = 5000;
        config.task_timeout_check_interval_ms = 1000;
        config.task_default_timeout_ms = 300000;
        config.node_health_timeout_ms = 30000;
        config.max_tasks_per_node = 10;
        config.max_retry_attempts = 3;
        config.enable_task_rebalancing = true;
        config.enable_auto_recovery = true;
        return config;
    }
};

struct ScheduleResult {
    std::string plan_id;
    std::vector<TaskAssignMessage> task_assignments;
    uint64_t schedule_time_ms;
    std::string schedule_id;
    std::map<std::string, std::string> metadata;
};

class MultiSatCoordinator : public IMessageHandler, public INodeRegistryListener {
public:
    explicit MultiSatCoordinator(const CoordinatorConfig& config);
    ~MultiSatCoordinator();
    
    MultiSatCoordinator(const MultiSatCoordinator&) = delete;
    MultiSatCoordinator& operator=(const MultiSatCoordinator&) = delete;
    
    bool initialize();
    void shutdown();
    bool start();
    void stop();
    
    bool submitScheduleResult(const ScheduleResult& schedule);
    
    // 加载全局配置
    bool loadGlobalConfig(const std::string& config_file);
    
    // 加载调度计划（所有卫星）
    bool loadSchedule(const std::string& schedule_file);
    
    // 预注册节点（根据配置文件中的卫星信息）
    bool preRegisterNodes();
    
    // 获取已加载的全局配置
    const GlobalConfigParser::GlobalConfig& getGlobalConfig() const { return global_config_; }
    
    // 获取已加载的调度计划
    const ScheduleParser::MultiSatSchedule& getSchedule() const { return schedule_data_; }
    
    bool getTaskStatus(const std::string& task_id, TaskRecord& task) const;
    std::vector<TaskRecord> getAllTasks() const;
    std::vector<TaskRecord> getTasksByNode(const std::string& node_id) const;
    std::vector<TaskRecord> getTasksByState(TaskLifecycleState state) const;
    
    bool cancelTask(const std::string& task_id, const std::string& reason);
    bool retryTask(const std::string& task_id);
    
    std::vector<NodeInfo> getHealthyNodes() const;
    std::vector<NodeInfo> getDegradedNodes() const;
    std::vector<NodeInfo> getOfflineNodes() const;
    
    void setMessageCallback(std::function<void(const Message&)> callback);
    void setTaskStateCallback(std::function<void(const std::string&, TaskLifecycleState)> callback);
    void setNodeHealthCallback(std::function<void(const std::string&, NodeStatus)> callback);
    
    std::shared_ptr<NodeRegistry> getNodeRegistry() { return node_registry_; }
    std::shared_ptr<InterSatComm> getComm() { return comm_; }
    
protected:
    void onMessageReceived(const Message& message) override;
    void onNodeConnected(const std::string& node_id) override;
    void onNodeDisconnected(const std::string& node_id, const std::string& reason) override;
    void onNodeStatusChanged(const std::string& node_id, NodeStatus old_status, NodeStatus new_status) override;
    
    void onNodeRegistered(const std::string& node_id) override;
    void onNodeUnregistered(const std::string& node_id, const std::string& reason) override;
    void onNodeTimeout(const std::string& node_id) override;
    void onNodeRecovered(const std::string& node_id) override;
    
    void onHeartbeatReceived(const std::string& node_id, const HeartbeatMessage& heartbeat) override;
    void onHeartbeatTimeout(const std::string& node_id) override;
    void onError(ErrorCode code, const std::string& message) override;
    
private:
    void mainLoop();
    void healthCheckLoop();
    void taskTimeoutCheckLoop();
    
    void processScheduleQueue();
    void distributeTask(const TaskAssignMessage& task_msg);
    std::string selectBestNode(const TaskAssignMessage& task_msg);
    
    void handleTaskAssignAck(const Message& message);
    void handleTaskProgress(const Message& message);
    void handleTaskComplete(const Message& message);
    void handleTaskAbort(const Message& message);
    void handleHeartbeat(const Message& message);
    
    void updateTaskState(const std::string& task_id, TaskLifecycleState new_state);
    void checkTaskTimeouts();
    void handleTaskTimeout(const std::string& task_id);
    
    void checkNodeHealth();
    void handleNodeDegraded(const std::string& node_id);
    void handleNodeFailure(const std::string& node_id);
    void reassignTasksFromNode(const std::string& node_id);
    
    uint64_t getCurrentTimeMs() const;
    void notifyTaskStateChanged(const std::string& task_id, TaskLifecycleState state);
    void notifyNodeHealthChanged(const std::string& node_id, NodeStatus status);
    
    CoordinatorConfig config_;
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    
    std::shared_ptr<InterSatComm> comm_;
    std::shared_ptr<NodeRegistry> node_registry_;
    
    std::map<std::string, TaskRecord> task_records_;
    mutable std::mutex task_mutex_;
    
    std::queue<ScheduleResult> schedule_queue_;
    std::mutex schedule_mutex_;
    std::condition_variable schedule_cv_;
    
    std::thread main_thread_;
    std::thread health_thread_;
    std::thread timeout_thread_;
    
    std::function<void(const Message&)> message_callback_;
    std::function<void(const std::string&, TaskLifecycleState)> task_state_callback_;
    std::function<void(const std::string&, NodeStatus)> node_health_callback_;
    
    std::map<std::string, uint32_t> node_task_counts_;
    mutable std::mutex node_task_mutex_;
    
    // 全局配置和调度数据
    GlobalConfigParser::GlobalConfig global_config_;
    ScheduleParser::MultiSatSchedule schedule_data_;
    mutable std::mutex config_mutex_;
};

}

#endif
