#include "multi_sat_coordinator.h"
#include "../input/json_parser.h"
#include <iostream>
#include <algorithm>

namespace coordinator {

MultiSatCoordinator::MultiSatCoordinator(const CoordinatorConfig& config)
    : config_(config),
      running_(false),
      initialized_(false),
      message_callback_(nullptr),
      task_state_callback_(nullptr),
      node_health_callback_(nullptr) {
}

MultiSatCoordinator::~MultiSatCoordinator() {
    shutdown();
}

bool MultiSatCoordinator::initialize() {
    if (initialized_.load()) {
        return true;
    }
    
    CommConfig comm_config = CommConfig::getDefault();
    comm_config.node_id = config_.coordinator_id;
    comm_config.node_name = config_.coordinator_name;
    comm_config.node_type = "COORDINATOR";
    
    comm_ = std::make_shared<InterSatComm>(comm_config);
    if (!comm_->initialize()) {
        return false;
    }
    
    node_registry_ = std::make_shared<NodeRegistry>(
        config_.node_health_timeout_ms,
        config_.health_check_interval_ms
    );
    
    if (!node_registry_->initialize()) {
        return false;
    }
    
    comm_->registerHandler(this);
    node_registry_->registerListener(this);
    
    initialized_.store(true);
    return true;
}

void MultiSatCoordinator::shutdown() {
    if (!initialized_.load()) {
        return;
    }
    
    stop();
    
    if (comm_) {
        comm_->unregisterHandler(this);
        comm_->stop();
    }
    
    if (node_registry_) {
        node_registry_->unregisterListener(this);
        node_registry_->shutdown();
    }
    
    initialized_.store(false);
}

bool MultiSatCoordinator::start() {
    if (!initialized_.load()) {
        return false;
    }
    
    if (running_.load()) {
        return true;
    }
    
    if (!comm_->start()) {
        return false;
    }
    
    running_.store(true);
    
    main_thread_ = std::thread(&MultiSatCoordinator::mainLoop, this);
    health_thread_ = std::thread(&MultiSatCoordinator::healthCheckLoop, this);
    timeout_thread_ = std::thread(&MultiSatCoordinator::taskTimeoutCheckLoop, this);
    
    return true;
}

void MultiSatCoordinator::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    schedule_cv_.notify_all();
    
    if (main_thread_.joinable()) {
        main_thread_.join();
    }
    
    if (health_thread_.joinable()) {
        health_thread_.join();
    }
    
    if (timeout_thread_.joinable()) {
        timeout_thread_.join();
    }
}

bool MultiSatCoordinator::submitScheduleResult(const ScheduleResult& schedule) {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    schedule_queue_.push(schedule);
    schedule_cv_.notify_one();
    return true;
}

bool MultiSatCoordinator::loadGlobalConfig(const std::string& config_file) {
    std::cout << "[Coordinator] 开始加载全局配置: " << config_file << std::endl;
    
    GlobalConfigParser parser;
    GlobalConfigParser::GlobalConfig config = parser.parse(config_file);
    
    if (config.plan_id.empty()) {
        std::cerr << "[Coordinator] 全局配置加载失败：plan_id为空" << std::endl;
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        global_config_ = config;
    }
    
    std::cout << "[Coordinator] 全局配置加载成功" << std::endl;
    std::cout << "  Plan ID: " << config.plan_id << std::endl;
    std::cout << "  活跃节点数: " << config.active_nodes.size() << std::endl;
    std::cout << "  信号量配置: " << config.semaphores.size() << " 个" << std::endl;
    std::cout << "  同步屏障: " << config.barriers.size() << " 个" << std::endl;
    std::cout << "  死锁检测: " << (config.deadlock_detection ? "启用" : "禁用") << std::endl;
    
    return true;
}

bool MultiSatCoordinator::loadSchedule(const std::string& schedule_file) {
    std::cout << "[Coordinator] 开始加载调度计划: " << schedule_file << std::endl;
    
    ScheduleParser parser;
    ScheduleParser::MultiSatSchedule schedule = parser.parseAllSatellites(schedule_file);
    
    if (schedule.satellite_ids.empty()) {
        std::cerr << "[Coordinator] 调度计划加载失败：未找到卫星数据" << std::endl;
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(config_mutex_);
        schedule_data_ = schedule;
    }
    
    std::cout << "[Coordinator] 调度计划加载成功" << std::endl;
    std::cout << "  Plan ID: " << schedule.plan_id << std::endl;
    std::cout << "  卫星数量: " << schedule.satellite_ids.size() << std::endl;
    
    // 统计并显示每颗卫星的任务数
    size_t total_tasks = 0;
    for (const auto& sat_id : schedule.satellite_ids) {
        auto it = schedule.satellite_tasks.find(sat_id);
        if (it != schedule.satellite_tasks.end()) {
            size_t task_count = it->second.size();
            total_tasks += task_count;
            std::cout << "    卫星 " << sat_id << ": " << task_count << " 个任务" << std::endl;
        }
    }
    std::cout << "  总任务数: " << total_tasks << std::endl;
    if (config_.enable_auto_recovery) {
        std::cout << "[Coordinator] 自动提交调度计划到任务队列" << std::endl;
        
        ScheduleResult result;
        result.plan_id = schedule.plan_id;
        result.schedule_id = schedule.schedule_id;
        result.schedule_time_ms = getCurrentTimeMs();
        
        // 将所有任务转换为TaskAssignMessage
        for (const auto& sat_pair : schedule.satellite_tasks) {
            const std::string& sat_id = sat_pair.first;
            const std::vector<TaskSegment>& tasks = sat_pair.second;
            
            for (const auto& task : tasks) {
                TaskAssignMessage task_msg;
                task_msg.segment_id = task.segment_id;
                task_msg.task_id = task.task_id;
                task_msg.task_name = task.behavior_ref;
                task_msg.behavior_ref = task.behavior_ref;
                task_msg.behavior_params = task.behavior_params;
                task_msg.priority = Priority::NORMAL;  // 可根据需要调整优先级
                
                // 设置执行时间窗口
                task_msg.execution.planned_start = task.execution.planned_start;
                task_msg.execution.planned_end = task.execution.planned_end;
                task_msg.execution.duration_s = task.execution.duration_s;
                
                // 设置时间窗口
                task_msg.window.window_id = task.window.window_id;
                task_msg.window.window_seq = task.window.window_seq;
                task_msg.window.start_time = task.window.start;
                task_msg.window.end_time = task.window.end;
                
                result.task_assignments.push_back(task_msg);
            }
        }
        
        submitScheduleResult(result);
        std::cout << "[Coordinator] 已提交 " << result.task_assignments.size() << " 个任务到队列" << std::endl;
    }
    
    // 自动预注册节点
    if (!schedule_data_.satellite_ids.empty()) {
        std::cout << "[Coordinator] 自动触发节点预注册" << std::endl;
        preRegisterNodes();
    }
    
    return true;
}

bool MultiSatCoordinator::preRegisterNodes() {
    if (!node_registry_) {
        std::cerr << "[Coordinator] 预注册失败：NodeRegistry未初始化" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(config_mutex_);
    
    // 合并所有需要预注册的节点ID
    std::set<std::string> all_node_ids;
    
    // 从全局配置中获取活跃节点
    for (const auto& node_id : global_config_.active_nodes) {
        all_node_ids.insert(node_id);
    }
    
    // 从调度数据中获取卫星ID
    for (const auto& sat_id : schedule_data_.satellite_ids) {
        all_node_ids.insert(sat_id);
    }
    
    if (all_node_ids.empty()) {
        std::cerr << "[Coordinator] 预注册失败：未找到需要注册的节点" << std::endl;
        return false;
    }
    
    std::cout << "[Coordinator] 开始预注册节点..." << std::endl;
    std::cout << "  待注册节点数: " << all_node_ids.size() << std::endl;
    
    int success_count = 0;
    int failed_count = 0;
    
    for (const auto& node_id : all_node_ids) {
        // 检查节点是否已经注册
        if (node_registry_->hasNode(node_id)) {
            std::cout << "  [" << node_id << "] 节点已存在，跳过" << std::endl;
            continue;
        }
        
        // 创建预注册消息
        NodeRegisterMessage register_msg;
        register_msg.node_id = node_id;
        register_msg.node_name = "Satellite_" + node_id;
        register_msg.node_type = "SATELLITE";
        register_msg.ip_address = "0.0.0.0";  // 预注册时未知IP
        register_msg.port = 0;  // 预注册时未知端口
        
        // 添加默认能力
        NodeCapability task_exec_cap;
        task_exec_cap.capability_id = "task_execution";
        task_exec_cap.description = "Execute scheduled tasks";
        register_msg.capabilities.push_back(task_exec_cap);
        
        NodeCapability imaging_cap;
        imaging_cap.capability_id = "imaging";
        imaging_cap.description = "Satellite imaging capability";
        register_msg.capabilities.push_back(imaging_cap);
        
        // 添加元数据
        register_msg.metadata["status"] = "pre_registered";
        register_msg.metadata["registration_type"] = "automatic";
        register_msg.metadata["source"] = "config_file";
        
        // 注册节点
        std::string assigned_id;
        uint64_t session_token;
        
        if (node_registry_->registerNode(register_msg, assigned_id, session_token)) {
            // 预注册的节点初始状态设为OFFLINE，等待实际连接后更新
            node_registry_->updateNodeStatus(assigned_id, NodeStatus::OFFLINE);
            
            std::cout << "  [" << node_id << "] 预注册成功 (Session: " 
                      << std::hex << session_token << std::dec << ")" << std::endl;
            success_count++;
        } else {
            std::cerr << "  [" << node_id << "] 预注册失败" << std::endl;
            failed_count++;
        }
    }
    
    std::cout << "[Coordinator] 预注册完成" << std::endl;
    std::cout << "  成功: " << success_count << " 个" << std::endl;
    std::cout << "  失败: " << failed_count << " 个" << std::endl;
    
    return (failed_count == 0);
}

bool MultiSatCoordinator::getTaskStatus(const std::string& task_id, TaskRecord& task) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(task_id);
    if (it == task_records_.end()) {
        return false;
    }
    task = it->second;
    return true;
}

std::vector<TaskRecord> MultiSatCoordinator::getAllTasks() const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    std::vector<TaskRecord> tasks;
    tasks.reserve(task_records_.size());
    for (const auto& pair : task_records_) {
        tasks.push_back(pair.second);
    }
    return tasks;
}

std::vector<TaskRecord> MultiSatCoordinator::getTasksByNode(const std::string& node_id) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    std::vector<TaskRecord> tasks;
    for (const auto& pair : task_records_) {
        if (pair.second.assigned_node_id == node_id) {
            tasks.push_back(pair.second);
        }
    }
    return tasks;
}

std::vector<TaskRecord> MultiSatCoordinator::getTasksByState(TaskLifecycleState state) const {
    std::lock_guard<std::mutex> lock(task_mutex_);
    std::vector<TaskRecord> tasks;
    for (const auto& pair : task_records_) {
        if (pair.second.state == state) {
            tasks.push_back(pair.second);
        }
    }
    return tasks;
}

bool MultiSatCoordinator::cancelTask(const std::string& task_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(task_id);
    if (it == task_records_.end()) {
        return false;
    }
    
    TaskRecord& task = it->second;
    if (task.state == TaskLifecycleState::COMPLETED ||
        task.state == TaskLifecycleState::CANCELLED ||
        task.state == TaskLifecycleState::FAILED) {
        return false;
    }
    
    TaskAbortMessage abort_msg;
    abort_msg.segment_id = task.segment_id;
    abort_msg.task_id = task.task_id;
    abort_msg.abort_reason = reason;
    abort_msg.recoverable = false;
    
    Message message;
    message.header.msg_type = MessageType::TASK_ABORT;
    message.header.source_node_id = config_.coordinator_id;
    message.header.dest_node_id = task.assigned_node_id;
    message.header.priority = Priority::URGENT;
    message.header.timestamp_ms = getCurrentTimeMs();
    
    if (comm_->sendMessage(task.assigned_node_id, message)) {
        task.state = TaskLifecycleState::CANCELLED;
        notifyTaskStateChanged(task_id, TaskLifecycleState::CANCELLED);
        return true;
    }
    
    return false;
}

bool MultiSatCoordinator::retryTask(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(task_id);
    if (it == task_records_.end()) {
        return false;
    }
    
    TaskRecord& task = it->second;
    if (task.retry_count >= task.max_retries) {
        return false;
    }
    
    task.retry_count++;
    task.state = TaskLifecycleState::CREATED;
    task.assign_time_ms = 0;
    task.start_time_ms = 0;
    task.progress_percent = 0;
    
    distributeTask(task.task_message);
    return true;
}

std::vector<NodeInfo> MultiSatCoordinator::getHealthyNodes() const {
    if (!node_registry_) {
        return {};
    }
    
    NodeQueryCriteria criteria;
    criteria.statuses.insert(NodeStatus::HEALTHY);
    criteria.only_online = true;
    
    std::vector<std::string> node_ids = node_registry_->queryNodes(criteria);
    std::vector<NodeInfo> nodes;
    for (const auto& node_id : node_ids) {
        NodeInfo info;
        if (node_registry_->getNodeInfo(node_id, info)) {
            nodes.push_back(info);
        }
    }
    return nodes;
}

std::vector<NodeInfo> MultiSatCoordinator::getDegradedNodes() const {
    if (!node_registry_) {
        return {};
    }
    
    NodeQueryCriteria criteria;
    criteria.statuses.insert(NodeStatus::DEGRADED);
    criteria.statuses.insert(NodeStatus::BUSY);
    criteria.only_online = true;
    
    std::vector<std::string> node_ids = node_registry_->queryNodes(criteria);
    std::vector<NodeInfo> nodes;
    for (const auto& node_id : node_ids) {
        NodeInfo info;
        if (node_registry_->getNodeInfo(node_id, info)) {
            nodes.push_back(info);
        }
    }
    return nodes;
}

std::vector<NodeInfo> MultiSatCoordinator::getOfflineNodes() const {
    if (!node_registry_) {
        return {};
    }
    
    NodeQueryCriteria criteria;
    criteria.statuses.insert(NodeStatus::OFFLINE);
    criteria.statuses.insert(NodeStatus::FAULT);
    
    std::vector<std::string> node_ids = node_registry_->queryNodes(criteria);
    std::vector<NodeInfo> nodes;
    for (const auto& node_id : node_ids) {
        NodeInfo info;
        if (node_registry_->getNodeInfo(node_id, info)) {
            nodes.push_back(info);
        }
    }
    return nodes;
}

void MultiSatCoordinator::setMessageCallback(std::function<void(const Message&)> callback) {
    message_callback_ = callback;
}

void MultiSatCoordinator::setTaskStateCallback(std::function<void(const std::string&, TaskLifecycleState)> callback) {
    task_state_callback_ = callback;
}

void MultiSatCoordinator::setNodeHealthCallback(std::function<void(const std::string&, NodeStatus)> callback) {
    node_health_callback_ = callback;
}

void MultiSatCoordinator::onMessageReceived(const Message& message) {
    if (message_callback_) {
        message_callback_(message);
    }
    
    switch (message.header.msg_type) {
        case MessageType::TASK_ASSIGN_ACK:
            handleTaskAssignAck(message);
            break;
        case MessageType::TASK_PROGRESS:
            handleTaskProgress(message);
            break;
        case MessageType::TASK_COMPLETE:
            handleTaskComplete(message);
            break;
        case MessageType::TASK_ABORT:
            handleTaskAbort(message);
            break;
        case MessageType::HEARTBEAT:
            handleHeartbeat(message);
            break;
        default:
            break;
    }
}

void MultiSatCoordinator::onNodeConnected(const std::string& node_id) {
    std::cout << "[Coordinator] 节点已连接: " << node_id << std::endl;
    
    // 如果节点是预注册的，更新其状态为HEALTHY
    if (node_registry_) {
        NodeInfo info;
        if (node_registry_->getNodeInfo(node_id, info)) {
            // 检查是否是预注册的节点
            auto it = info.metadata.find("registration_type");
            if (it != info.metadata.end() && it->second == "automatic") {
                std::cout << "[Coordinator] 预注册节点 " << node_id << " 已上线，更新状态" << std::endl;
                node_registry_->updateNodeStatus(node_id, NodeStatus::HEALTHY);
                node_registry_->markNodeOnline(node_id);
            }
        }
    }
}

void MultiSatCoordinator::onNodeDisconnected(const std::string& node_id, const std::string& reason) {
    handleNodeFailure(node_id);
}

void MultiSatCoordinator::onNodeStatusChanged(const std::string& node_id, NodeStatus old_status, NodeStatus new_status) {
    notifyNodeHealthChanged(node_id, new_status);
    
    if (new_status == NodeStatus::DEGRADED || new_status == NodeStatus::BUSY) {
        handleNodeDegraded(node_id);
    } else if (new_status == NodeStatus::OFFLINE || new_status == NodeStatus::FAULT) {
        handleNodeFailure(node_id);
    }
}

void MultiSatCoordinator::onNodeRegistered(const std::string& node_id) {
    std::lock_guard<std::mutex> lock(node_task_mutex_);
    node_task_counts_[node_id] = 0;
}

void MultiSatCoordinator::onNodeUnregistered(const std::string& node_id, const std::string& reason) {
    reassignTasksFromNode(node_id);
    
    std::lock_guard<std::mutex> lock(node_task_mutex_);
    node_task_counts_.erase(node_id);
}

void MultiSatCoordinator::onNodeTimeout(const std::string& node_id) {
    handleNodeFailure(node_id);
}

void MultiSatCoordinator::onNodeRecovered(const std::string& node_id) {
    notifyNodeHealthChanged(node_id, NodeStatus::HEALTHY);
}

void MultiSatCoordinator::mainLoop() {
    while (running_.load()) {
        processScheduleQueue();
        
        std::unique_lock<std::mutex> lock(schedule_mutex_);
        schedule_cv_.wait_for(lock, std::chrono::milliseconds(config_.main_loop_interval_ms));
    }
}

void MultiSatCoordinator::healthCheckLoop() {
    while (running_.load()) {
        checkNodeHealth();
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.health_check_interval_ms));
    }
}

void MultiSatCoordinator::taskTimeoutCheckLoop() {
    while (running_.load()) {
        checkTaskTimeouts();
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.task_timeout_check_interval_ms));
    }
}

void MultiSatCoordinator::processScheduleQueue() {
    std::unique_lock<std::mutex> lock(schedule_mutex_);
    
    while (!schedule_queue_.empty()) {
        ScheduleResult schedule = schedule_queue_.front();
        schedule_queue_.pop();
        lock.unlock();
        
        for (const auto& task_msg : schedule.task_assignments) {
            distributeTask(task_msg);
        }
        
        lock.lock();
    }
}

void MultiSatCoordinator::distributeTask(const TaskAssignMessage& task_msg) {
    std::string best_node = selectBestNode(task_msg);
    
    if (best_node.empty()) {
        std::lock_guard<std::mutex> lock(task_mutex_);
        TaskRecord task;
        task.segment_id = task_msg.segment_id;
        task.task_id = task_msg.task_id;
        task.state = TaskLifecycleState::CREATED;
        task.priority = task_msg.priority;
        task.task_message = task_msg;
        task.create_time_ms = getCurrentTimeMs();
        task.timeout_ms = config_.task_default_timeout_ms;
        task_records_[task_msg.task_id] = task;
        
        notifyTaskStateChanged(task_msg.task_id, TaskLifecycleState::CREATED);
        return;
    }
    
    Message message;
    message.header.msg_type = MessageType::TASK_ASSIGN;
    message.header.source_node_id = config_.coordinator_id;
    message.header.dest_node_id = best_node;
    message.header.priority = task_msg.priority;
    message.header.timestamp_ms = getCurrentTimeMs();
    static std::atomic<uint32_t> seq_id(0);
    message.header.sequence_id = seq_id.fetch_add(1);
    message.payload = MessageSerializer::serializeTaskAssign(task_msg);
    
    if (comm_->sendMessage(best_node, message)) {
        std::lock_guard<std::mutex> lock(task_mutex_);
        
        TaskRecord task;
        task.segment_id = task_msg.segment_id;
        task.task_id = task_msg.task_id;
        task.assigned_node_id = best_node;
        task.state = TaskLifecycleState::ASSIGNED;
        task.priority = task_msg.priority;
        task.task_message = task_msg;
        task.create_time_ms = getCurrentTimeMs();
        task.assign_time_ms = getCurrentTimeMs();
        task.timeout_ms = config_.task_default_timeout_ms;
        task_records_[task_msg.task_id] = task;
        
        std::lock_guard<std::mutex> node_lock(node_task_mutex_);
        node_task_counts_[best_node]++;
        
        notifyTaskStateChanged(task_msg.task_id, TaskLifecycleState::ASSIGNED);
    }
}

std::string MultiSatCoordinator::selectBestNode(const TaskAssignMessage& task_msg) {
    NodeQueryCriteria criteria;
    criteria.statuses.insert(NodeStatus::HEALTHY);
    criteria.only_online = true;
    
    std::vector<std::string> node_ids = node_registry_->queryNodes(criteria);
    if (node_ids.empty()) {
        return "";
    }
    
    std::vector<NodeInfo> nodes;
    for (const auto& node_id : node_ids) {
        NodeInfo info;
        if (node_registry_->getNodeInfo(node_id, info)) {
            nodes.push_back(info);
        }
    }
    
    std::string best_node;
    uint32_t min_task_count = UINT32_MAX;
    
    std::lock_guard<std::mutex> lock(node_task_mutex_);
    
    for (const auto& node : nodes) {
        uint32_t task_count = 0;
        auto it = node_task_counts_.find(node.node_id);
        if (it != node_task_counts_.end()) {
            task_count = it->second;
        }
        
        if (task_count >= config_.max_tasks_per_node) {
            continue;
        }
        
        if (task_count < min_task_count) {
            min_task_count = task_count;
            best_node = node.node_id;
        }
    }
    
    return best_node;
}

void MultiSatCoordinator::handleTaskAssignAck(const Message& message) {
    TaskAssignAck ack;
    if (!MessageSerializer::deserializeTaskAssignAck(message.payload, ack)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(ack.task_id);
    if (it == task_records_.end()) {
        return;
    }
    
    TaskRecord& task = it->second;
    
    if (ack.accepted) {
        task.state = TaskLifecycleState::ACKNOWLEDGED;
        notifyTaskStateChanged(ack.task_id, TaskLifecycleState::ACKNOWLEDGED);
    } else {
        task.state = TaskLifecycleState::FAILED;
        task.result_summary = ack.reject_reason;
        
        std::lock_guard<std::mutex> node_lock(node_task_mutex_);
        if (node_task_counts_.find(task.assigned_node_id) != node_task_counts_.end()) {
            node_task_counts_[task.assigned_node_id]--;
        }
        
        notifyTaskStateChanged(ack.task_id, TaskLifecycleState::FAILED);
        
        if (config_.enable_auto_recovery && task.retry_count < task.max_retries) {
            task.retry_count++;
            task.state = TaskLifecycleState::CREATED;
            task.assigned_node_id.clear();
            task.assign_time_ms = 0;
            distributeTask(task.task_message);
        }
    }
}

void MultiSatCoordinator::handleTaskProgress(const Message& message) {
    TaskProgressMessage progress;
    if (!MessageSerializer::deserializeTaskProgress(message.payload, progress)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(progress.task_id);
    if (it == task_records_.end()) {
        return;
    }
    
    TaskRecord& task = it->second;
    
    if (progress.status == TaskStatus::RUNNING) {
        if (task.state != TaskLifecycleState::EXECUTING) {
            task.state = TaskLifecycleState::EXECUTING;
            task.start_time_ms = getCurrentTimeMs();
            notifyTaskStateChanged(progress.task_id, TaskLifecycleState::EXECUTING);
        }
    } else if (progress.status == TaskStatus::QUEUED) {
        task.state = TaskLifecycleState::QUEUED;
        notifyTaskStateChanged(progress.task_id, TaskLifecycleState::QUEUED);
    }
    
    task.progress_percent = progress.progress_percent;
    task.current_action = progress.current_action;
}

void MultiSatCoordinator::handleTaskComplete(const Message& message) {
    TaskCompleteMessage complete;
    if (!MessageSerializer::deserializeTaskComplete(message.payload, complete)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(complete.task_id);
    if (it == task_records_.end()) {
        return;
    }
    
    TaskRecord& task = it->second;
    task.complete_time_ms = getCurrentTimeMs();
    task.success = complete.success;
    task.result_summary = complete.result_summary;
    task.progress_percent = 100;
    
    if (complete.success) {
        task.state = TaskLifecycleState::COMPLETED;
        notifyTaskStateChanged(complete.task_id, TaskLifecycleState::COMPLETED);
    } else {
        task.state = TaskLifecycleState::FAILED;
        notifyTaskStateChanged(complete.task_id, TaskLifecycleState::FAILED);
    }
    
    std::lock_guard<std::mutex> node_lock(node_task_mutex_);
    if (node_task_counts_.find(task.assigned_node_id) != node_task_counts_.end()) {
        node_task_counts_[task.assigned_node_id]--;
    }
}

void MultiSatCoordinator::handleTaskAbort(const Message& message) {
    TaskAbortMessage abort;
    std::vector<uint8_t> data = message.payload;
    if (data.empty()) {
        return;
    }
    abort.segment_id = "";
    abort.task_id = "";
    abort.abort_reason = "";
    abort.recoverable = false;
    
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(abort.task_id);
    if (it == task_records_.end()) {
        return;
    }
    
    TaskRecord& task = it->second;
    task.state = TaskLifecycleState::FAILED;
    task.result_summary = abort.abort_reason;
    
    std::lock_guard<std::mutex> node_lock(node_task_mutex_);
    if (node_task_counts_.find(task.assigned_node_id) != node_task_counts_.end()) {
        node_task_counts_[task.assigned_node_id]--;
    }
    
    notifyTaskStateChanged(abort.task_id, TaskLifecycleState::FAILED);
    
    if (config_.enable_auto_recovery && abort.recoverable && task.retry_count < task.max_retries) {
        task.retry_count++;
        task.state = TaskLifecycleState::CREATED;
        task.assigned_node_id.clear();
        task.assign_time_ms = 0;
        distributeTask(task.task_message);
    }
}

void MultiSatCoordinator::handleHeartbeat(const Message& message) {
    HeartbeatMessage heartbeat;
    if (!MessageSerializer::deserializeHeartbeat(message.payload, heartbeat)) {
        return;
    }
    
    if (node_registry_) {
        node_registry_->updateHeartbeat(heartbeat.node_id, heartbeat);
    }
}

void MultiSatCoordinator::updateTaskState(const std::string& task_id, TaskLifecycleState new_state) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(task_id);
    if (it != task_records_.end()) {
        it->second.state = new_state;
        notifyTaskStateChanged(task_id, new_state);
    }
}

void MultiSatCoordinator::checkTaskTimeouts() {
    uint64_t current_time = getCurrentTimeMs();
    std::vector<std::string> timeout_tasks;
    
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        for (const auto& pair : task_records_) {
            const TaskRecord& task = pair.second;
            
            if (task.state != TaskLifecycleState::EXECUTING &&
                task.state != TaskLifecycleState::ASSIGNED &&
                task.state != TaskLifecycleState::ACKNOWLEDGED &&
                task.state != TaskLifecycleState::QUEUED) {
                continue;
            }
            
            uint64_t elapsed = current_time - task.assign_time_ms;
            if (elapsed > task.timeout_ms) {
                timeout_tasks.push_back(task.task_id);
            }
        }
    }
    
    for (const auto& task_id : timeout_tasks) {
        handleTaskTimeout(task_id);
    }
}

void MultiSatCoordinator::handleTaskTimeout(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(task_mutex_);
    auto it = task_records_.find(task_id);
    if (it == task_records_.end()) {
        return;
    }
    
    TaskRecord& task = it->second;
    task.state = TaskLifecycleState::TIMEOUT;
    
    std::lock_guard<std::mutex> node_lock(node_task_mutex_);
    if (node_task_counts_.find(task.assigned_node_id) != node_task_counts_.end()) {
        node_task_counts_[task.assigned_node_id]--;
    }
    
    notifyTaskStateChanged(task_id, TaskLifecycleState::TIMEOUT);
    
    if (config_.enable_auto_recovery && task.retry_count < task.max_retries) {
        task.retry_count++;
        task.state = TaskLifecycleState::CREATED;
        task.assigned_node_id.clear();
        task.assign_time_ms = 0;
        distributeTask(task.task_message);
    }
}

void MultiSatCoordinator::checkNodeHealth() {
    if (!node_registry_) {
        return;
    }
    
    std::vector<std::string> node_ids = node_registry_->getAllNodeIds();
    if (node_ids.empty()) {
        return;
    }
    
    std::vector<NodeInfo> all_nodes;
    for (const auto& node_id : node_ids) {
        NodeInfo info;
        if (node_registry_->getNodeInfo(node_id, info)) {
            all_nodes.push_back(info);
        }
    }
    
    uint64_t current_time = getCurrentTimeMs();
    
    for (const auto& node : all_nodes) {
        if (!node.is_online) {
            continue;
        }
        
        uint64_t time_since_heartbeat = current_time - node.last_heartbeat_ms;
        
        if (time_since_heartbeat > config_.node_health_timeout_ms) {
            if (node.status != NodeStatus::OFFLINE && node.status != NodeStatus::FAULT) {
                node_registry_->updateNodeStatus(node.node_id, NodeStatus::OFFLINE);
            }
        } else if (node.status == NodeStatus::HEALTHY) {
            if (node.stats.cpu_usage_percent > 90 || node.stats.memory_usage_percent > 90) {
                node_registry_->updateNodeStatus(node.node_id, NodeStatus::DEGRADED);
            } else if (node.stats.active_tasks >= config_.max_tasks_per_node) {
                node_registry_->updateNodeStatus(node.node_id, NodeStatus::BUSY);
            }
        }
    }
}

void MultiSatCoordinator::handleNodeDegraded(const std::string& node_id) {
    if (!config_.enable_task_rebalancing) {
        return;
    }
}

void MultiSatCoordinator::handleNodeFailure(const std::string& node_id) {
    if (config_.enable_auto_recovery) {
        reassignTasksFromNode(node_id);
    }
}

void MultiSatCoordinator::reassignTasksFromNode(const std::string& node_id) {
    std::vector<TaskRecord> tasks_to_reassign;
    
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        for (auto& pair : task_records_) {
            TaskRecord& task = pair.second;
            
            if (task.assigned_node_id == node_id &&
                (task.state == TaskLifecycleState::ASSIGNED ||
                 task.state == TaskLifecycleState::ACKNOWLEDGED ||
                 task.state == TaskLifecycleState::QUEUED ||
                 task.state == TaskLifecycleState::EXECUTING)) {
                
                if (task.retry_count < task.max_retries) {
                    tasks_to_reassign.push_back(task);
                    task.state = TaskLifecycleState::CREATED;
                    task.assigned_node_id.clear();
                    task.assign_time_ms = 0;
                    task.retry_count++;
                } else {
                    task.state = TaskLifecycleState::FAILED;
                    task.result_summary = "Node failure, max retries exceeded";
                    notifyTaskStateChanged(task.task_id, TaskLifecycleState::FAILED);
                }
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(node_task_mutex_);
        node_task_counts_[node_id] = 0;
    }
    
    for (const auto& task : tasks_to_reassign) {
        distributeTask(task.task_message);
    }
}

uint64_t MultiSatCoordinator::getCurrentTimeMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void MultiSatCoordinator::notifyTaskStateChanged(const std::string& task_id, TaskLifecycleState state) {
    if (task_state_callback_) {
        task_state_callback_(task_id, state);
    }
}

void MultiSatCoordinator::notifyNodeHealthChanged(const std::string& node_id, NodeStatus status) {
    if (node_health_callback_) {
        node_health_callback_(node_id, status);
    }
}

void MultiSatCoordinator::onHeartbeatReceived(const std::string& node_id, const HeartbeatMessage& heartbeat) {
}

void MultiSatCoordinator::onHeartbeatTimeout(const std::string& node_id) {
    handleNodeFailure(node_id);
}

void MultiSatCoordinator::onError(ErrorCode code, const std::string& message) {
}

}
