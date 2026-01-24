// #include "inter_sat_comm.h"
// #include "../third_party/nlohmann/json.hpp"
// #include <iostream>
// #include <sstream>
// #include <cstring>
// #include <algorithm>
// #ifdef _WIN32
//     #include <winsock2.h>
//     #include <ws2tcpip.h>
//     #pragma comment(lib, "ws2_32.lib")
//     typedef int socklen_t;
//     #ifndef INVALID_SOCKET_VALUE
//         #define INVALID_SOCKET_VALUE INVALID_SOCKET
//     #endif
// #else
//     #include <sys/socket.h>
//     #include <netinet/in.h>
//     #include <arpa/inet.h>
//     #include <unistd.h>
//     #include <fcntl.h>
//     #ifndef INVALID_SOCKET_VALUE
//         #define INVALID_SOCKET_VALUE (-1)
//     #endif
//     #define SOCKET_ERROR -1
//     #define closesocket close
// #endif
// using json = nlohmann::json;

// namespace coordinator {

// // MessageQueue 实现
// MessageQueue::MessageQueue(size_t max_size) : max_size_(max_size) {}
// MessageQueue::~MessageQueue() {
//     clear();
// }
// bool MessageQueue::push(const Message& message) {
//     std::unique_lock<std::mutex> lock(mutex_);
//     if (queue_.size() >= max_size_) {
//         return false;
//     }
//     PriorityMessage pm;
//     pm.message = message;
//     pm.priority = static_cast<int>(message.header.priority);
//     pm.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()).count();
//     queue_.push(pm);
//     cv_.notify_one();
//     return true;
// }
// bool MessageQueue::pushPriority(const Message& message) {
//     std::unique_lock<std::mutex> lock(mutex_);
//     if (queue_.size() >= max_size_) {
//         return false;
//     }
//     PriorityMessage pm;
//     pm.message = message;
//     pm.priority = 0; 
//     pm.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()).count();
//     queue_.push(pm);
//     cv_.notify_one();
//     return true;
// }
// bool MessageQueue::pop(Message& message, uint32_t timeout_ms) {
//     std::unique_lock<std::mutex> lock(mutex_);
//     if (timeout_ms == 0) {
//         cv_.wait(lock, [this] { return !queue_.empty(); });
//     } else {
//         if (!cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
//                          [this] { return !queue_.empty(); })) {
//             return false;
//         }
//     }
//     if (queue_.empty()) {
//         return false;
//     }
//     message = queue_.top().message;
//     queue_.pop();
//     return true;
// }
// bool MessageQueue::tryPop(Message& message) {
//     std::unique_lock<std::mutex> lock(mutex_);
//     if (queue_.empty()) {
//         return false;
//     }
//     message = queue_.top().message;
//     queue_.pop();
//     return true;
// }
// size_t MessageQueue::size() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     return queue_.size();
// }
// bool MessageQueue::empty() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     return queue_.empty();
// }
// bool MessageQueue::full() const {
//     std::lock_guard<std::mutex> lock(mutex_);
//     return queue_.size() >= max_size_;
// }
// void MessageQueue::clear() {
//     std::lock_guard<std::mutex> lock(mutex_);
//     while (!queue_.empty()) {
//         queue_.pop();
//     }
// }
// // MessageSerializer 实现
// std::vector<uint8_t> MessageSerializer::serialize(const Message& message) {
//     std::vector<uint8_t> result;
//     auto header_data = serializeHeader(message.header);
//     result.insert(result.end(), header_data.begin(), header_data.end());
//     result.insert(result.end(), message.payload.begin(), message.payload.end());
//     return result;
// }
// std::vector<uint8_t> MessageSerializer::serializeHeader(const MessageHeader& header) {
//     std::vector<uint8_t> data;
//     data.push_back((header.magic >> 24) & 0xFF);
//     data.push_back((header.magic >> 16) & 0xFF);
//     data.push_back((header.magic >> 8) & 0xFF);
//     data.push_back(header.magic & 0xFF);
//     data.push_back((header.version >> 8) & 0xFF);
//     data.push_back(header.version & 0xFF);
//     uint16_t msg_type = static_cast<uint16_t>(header.msg_type);
//     data.push_back((msg_type >> 8) & 0xFF);
//     data.push_back(msg_type & 0xFF);
//     data.push_back((header.sequence_id >> 24) & 0xFF);
//     data.push_back((header.sequence_id >> 16) & 0xFF);
//     data.push_back((header.sequence_id >> 8) & 0xFF);
//     data.push_back(header.sequence_id & 0xFF);
//     data.push_back((header.timestamp_ms >> 56) & 0xFF);
//     data.push_back((header.timestamp_ms >> 48) & 0xFF);
//     data.push_back((header.timestamp_ms >> 40) & 0xFF);
//     data.push_back((header.timestamp_ms >> 32) & 0xFF);
//     data.push_back((header.timestamp_ms >> 24) & 0xFF);
//     data.push_back((header.timestamp_ms >> 16) & 0xFF);
//     data.push_back((header.timestamp_ms >> 8) & 0xFF);
//     data.push_back(header.timestamp_ms & 0xFF);
//     uint8_t src_len = static_cast<uint8_t>(header.source_node_id.length());
//     data.push_back(src_len);
//     data.insert(data.end(), header.source_node_id.begin(), header.source_node_id.end());
//     uint8_t dst_len = static_cast<uint8_t>(header.dest_node_id.length());
//     data.push_back(dst_len);
//     data.insert(data.end(), header.dest_node_id.begin(), header.dest_node_id.end());
//     data.push_back(static_cast<uint8_t>(header.priority));
//     data.push_back((header.payload_size >> 24) & 0xFF);
//     data.push_back((header.payload_size >> 16) & 0xFF);
//     data.push_back((header.payload_size >> 8) & 0xFF);
//     data.push_back(header.payload_size & 0xFF);
//     data.push_back((header.checksum >> 24) & 0xFF);
//     data.push_back((header.checksum >> 16) & 0xFF);
//     data.push_back((header.checksum >> 8) & 0xFF);
//     data.push_back(header.checksum & 0xFF);
//     return data;
// }

// bool MessageSerializer::deserialize(const std::vector<uint8_t>& data, Message& message) {
//     if (data.size() < 32) { 
//         return false;
//     }
//     size_t offset = 0;
//     if (!deserializeHeader(data, message.header)) {
//         return false;
//     }
//     // Header 实际大小: 4(magic) + 2(version) + 2(msg_type) + 4(seq_id) + 8(timestamp)
//     //                + 1(src_len) + src_len + 1(dst_len) + dst_len
//     //                + 1(priority) + 4(payload_size) + 4(checksum)
//     // = 31 + src_len + dst_len
//     offset = 31 + message.header.source_node_id.length() + message.header.dest_node_id.length();
//     if (offset + message.header.payload_size <= data.size()) {
//         message.payload.assign(data.begin() + offset, 
//                               data.begin() + offset + message.header.payload_size);
//     } 
//     return true;
// }
// bool MessageSerializer::deserializeHeader(const std::vector<uint8_t>& data, MessageHeader& header) {
//     if (data.size() < 32) {
//         return false;
//     }
//     size_t offset = 0;
//     header.magic = (static_cast<uint32_t>(data[offset++]) << 24) |
//                    (static_cast<uint32_t>(data[offset++]) << 16) |
//                    (static_cast<uint32_t>(data[offset++]) << 8) |
//                    static_cast<uint32_t>(data[offset++]);
//     header.version = (static_cast<uint16_t>(data[offset++]) << 8) |
//                      static_cast<uint16_t>(data[offset++]);
//     uint16_t msg_type = (static_cast<uint16_t>(data[offset++]) << 8) |
//                         static_cast<uint16_t>(data[offset++]);
//     header.msg_type = static_cast<MessageType>(msg_type);
//     header.sequence_id = (static_cast<uint32_t>(data[offset++]) << 24) |
//                         (static_cast<uint32_t>(data[offset++]) << 16) |
//                         (static_cast<uint32_t>(data[offset++]) << 8) |
//                         static_cast<uint32_t>(data[offset++]);
//     header.timestamp_ms = (static_cast<uint64_t>(data[offset++]) << 56) |
//                          (static_cast<uint64_t>(data[offset++]) << 48) |
//                          (static_cast<uint64_t>(data[offset++]) << 40) |
//                          (static_cast<uint64_t>(data[offset++]) << 32) |
//                          (static_cast<uint64_t>(data[offset++]) << 24) |
//                          (static_cast<uint64_t>(data[offset++]) << 16) |
//                          (static_cast<uint64_t>(data[offset++]) << 8) |
//                          static_cast<uint64_t>(data[offset++]);
//     uint8_t src_len = data[offset++];
//     if (offset + src_len > data.size()) return false;
//     header.source_node_id.assign(data.begin() + offset, data.begin() + offset + src_len);
//     offset += src_len;
//     uint8_t dst_len = data[offset++];
//     if (offset + dst_len > data.size()) return false;
//     header.dest_node_id.assign(data.begin() + offset, data.begin() + offset + dst_len);
//     offset += dst_len;
//     header.priority = static_cast<Priority>(data[offset++]);
//     header.payload_size = (static_cast<uint32_t>(data[offset++]) << 24) |
//                          (static_cast<uint32_t>(data[offset++]) << 16) |
//                          (static_cast<uint32_t>(data[offset++]) << 8) |
//                          static_cast<uint32_t>(data[offset++]);
//     header.checksum = (static_cast<uint32_t>(data[offset++]) << 24) |
//                      (static_cast<uint32_t>(data[offset++]) << 16) |
//                      (static_cast<uint32_t>(data[offset++]) << 8) |
//                      static_cast<uint32_t>(data[offset++]);
    
//     return true;
// }
// std::vector<uint8_t> MessageSerializer::serializeHeartbeat(const HeartbeatMessage& msg) {
//     json j;
//     j["node_id"] = msg.node_id;
//     j["uptime_ms"] = msg.uptime_ms;
//     j["status"] = static_cast<int>(msg.status);
//     j["cpu_usage"] = msg.cpu_usage_percent;
//     j["mem_usage"] = msg.memory_usage_percent;
//     j["battery"] = msg.battery_percent;
//     j["active_tasks"] = msg.active_tasks;
//     j["pending_messages"] = msg.pending_messages;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }
// bool MessageSerializer::deserializeHeartbeat(const std::vector<uint8_t>& data, HeartbeatMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.node_id = j["node_id"].get<std::string>();
//         msg.uptime_ms = j["uptime_ms"].get<uint64_t>();
//         msg.status = static_cast<NodeStatus>(j["status"].get<int>());
//         msg.cpu_usage_percent = j["cpu_usage"].get<uint8_t>();
//         msg.memory_usage_percent = j["mem_usage"].get<uint8_t>();
//         msg.battery_percent = j["battery"].get<int8_t>();
//         msg.active_tasks = j["active_tasks"].get<uint32_t>();
//         msg.pending_messages = j["pending_messages"].get<uint32_t>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }
// std::vector<uint8_t> MessageSerializer::serializeHeartbeatAck(const HeartbeatAck& msg) {
//     json j;
//     j["node_id"] = msg.node_id;
//     j["server_time_ms"] = msg.server_time_ms;
//     j["sync_required"] = msg.sync_required;
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }
// bool MessageSerializer::deserializeHeartbeatAck(const std::vector<uint8_t>& data, HeartbeatAck& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.node_id = j["node_id"].get<std::string>();
//         msg.server_time_ms = j["server_time_ms"].get<uint64_t>();
//         msg.sync_required = j["sync_required"].get<bool>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }
// std::vector<uint8_t> MessageSerializer::serializeNodeRegister(const NodeRegisterMessage& msg) {
//     json j;
//     j["node_id"] = msg.node_id;
//     j["node_name"] = msg.node_name;
//     j["node_type"] = msg.node_type;
//     j["ip_address"] = msg.ip_address;
//     j["port"] = msg.port;
//     json caps = json::array();
//     for (const auto& cap : msg.capabilities) {
//         json cap_j;
//         cap_j["capability_id"] = cap.capability_id;
//         cap_j["description"] = cap.description;
//         cap_j["params"] = cap.params;
//         caps.push_back(cap_j);
//     }
//     j["capabilities"] = caps;
//     j["metadata"] = msg.metadata;
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }
// bool MessageSerializer::deserializeNodeRegister(const std::vector<uint8_t>& data, NodeRegisterMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
//         msg.node_id = j["node_id"].get<std::string>();
//         msg.node_name = j["node_name"].get<std::string>();
//         msg.node_type = j["node_type"].get<std::string>();
//         msg.ip_address = j["ip_address"].get<std::string>();
//         msg.port = j["port"].get<uint16_t>();
//         msg.capabilities.clear();
//         for (const auto& cap_j : j["capabilities"]) {
//             NodeCapability cap;
//             cap.capability_id = cap_j["capability_id"].get<std::string>();
//             cap.description = cap_j["description"].get<std::string>();
//             cap.params = cap_j["params"].get<std::map<std::string, std::string>>();
//             msg.capabilities.push_back(cap);
//         }
        
//         msg.metadata = j["metadata"].get<std::map<std::string, std::string>>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeNodeRegisterAck(const NodeRegisterAck& msg) {
//     json j;
//     j["success"] = msg.success;
//     j["assigned_node_id"] = msg.assigned_node_id;
//     j["coordinator_id"] = msg.coordinator_id;
//     j["session_token"] = msg.session_token;
//     j["heartbeat_interval_ms"] = msg.heartbeat_interval_ms;
//     j["error_message"] = msg.error_message;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeNodeRegisterAck(const std::vector<uint8_t>& data, NodeRegisterAck& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.success = j["success"].get<bool>();
//         msg.assigned_node_id = j["assigned_node_id"].get<std::string>();
//         msg.coordinator_id = j["coordinator_id"].get<std::string>();
//         msg.session_token = j["session_token"].get<uint64_t>();
//         msg.heartbeat_interval_ms = j["heartbeat_interval_ms"].get<uint32_t>();
//         msg.error_message = j["error_message"].get<std::string>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeTaskAssign(const TaskAssignMessage& msg) {
//     json j;
//     j["segment_id"] = msg.segment_id;
//     j["task_id"] = msg.task_id;
//     j["task_name"] = msg.task_name;
//     j["priority"] = static_cast<int>(msg.priority);
//     j["profit"] = msg.profit;
    
//     j["target"]["latitude"] = msg.target.latitude_deg;
//     j["target"]["longitude"] = msg.target.longitude_deg;
//     j["target"]["name"] = msg.target.target_name;
    
//     j["window"]["window_id"] = msg.window.window_id;
//     j["window"]["window_seq"] = msg.window.window_seq;
//     j["window"]["start_time"] = msg.window.start_time;
//     j["window"]["end_time"] = msg.window.end_time;
    
//     j["execution"]["planned_start"] = msg.execution.planned_start;
//     j["execution"]["planned_end"] = msg.execution.planned_end;
//     j["execution"]["duration_s"] = msg.execution.duration_s;
    
//     j["behavior_ref"] = msg.behavior_ref;
//     j["behavior_params"] = msg.behavior_params;
    
//     json resources = json::array();
//     for (const auto& res : msg.resource_requirements) {
//         json res_j;
//         res_j["resource_id"] = res.resource_id;
//         res_j["semaphore_id"] = res.semaphore_id;
//         res_j["usage_type"] = res.usage_type;
//         res_j["acquire_at"] = res.acquire_at;
//         res_j["hold_duration_s"] = res.hold_duration_s;
//         resources.push_back(res_j);
//     }
//     j["resource_requirements"] = resources;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeTaskAssign(const std::vector<uint8_t>& data, TaskAssignMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.segment_id = j["segment_id"].get<std::string>();
//         msg.task_id = j["task_id"].get<std::string>();
//         msg.task_name = j["task_name"].get<std::string>();
//         msg.priority = static_cast<Priority>(j["priority"].get<int>());
//         msg.profit = j["profit"].get<int>();
        
//         msg.target.latitude_deg = j["target"]["latitude"].get<double>();
//         msg.target.longitude_deg = j["target"]["longitude"].get<double>();
//         msg.target.target_name = j["target"]["name"].get<std::string>();
        
//         msg.window.window_id = j["window"]["window_id"].get<std::string>();
//         msg.window.window_seq = j["window"]["window_seq"].get<int>();
//         msg.window.start_time = j["window"]["start_time"].get<std::string>();
//         msg.window.end_time = j["window"]["end_time"].get<std::string>();
        
//         msg.execution.planned_start = j["execution"]["planned_start"].get<std::string>();
//         msg.execution.planned_end = j["execution"]["planned_end"].get<std::string>();
//         msg.execution.duration_s = j["execution"]["duration_s"].get<int>();
        
//         msg.behavior_ref = j["behavior_ref"].get<std::string>();
//         msg.behavior_params = j["behavior_params"].get<std::map<std::string, std::string>>();
        
//         msg.resource_requirements.clear();
//         for (const auto& res_j : j["resource_requirements"]) {
//             ResourceRequirement res;
//             res.resource_id = res_j["resource_id"].get<std::string>();
//             res.semaphore_id = res_j["semaphore_id"].get<std::string>();
//             res.usage_type = res_j["usage_type"].get<std::string>();
//             res.acquire_at = res_j["acquire_at"].get<std::string>();
//             res.hold_duration_s = res_j["hold_duration_s"].get<int>();
//             msg.resource_requirements.push_back(res);
//         }
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeTaskAssignAck(const TaskAssignAck& msg) {
//     json j;
//     j["segment_id"] = msg.segment_id;
//     j["task_id"] = msg.task_id;
//     j["accepted"] = msg.accepted;
//     j["reject_reason"] = msg.reject_reason;
//     j["estimated_start"] = msg.estimated_start;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeTaskAssignAck(const std::vector<uint8_t>& data, TaskAssignAck& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.segment_id = j["segment_id"].get<std::string>();
//         msg.task_id = j["task_id"].get<std::string>();
//         msg.accepted = j["accepted"].get<bool>();
//         msg.reject_reason = j["reject_reason"].get<std::string>();
//         msg.estimated_start = j["estimated_start"].get<std::string>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeTaskProgress(const TaskProgressMessage& msg) {
//     json j;
//     j["segment_id"] = msg.segment_id;
//     j["task_id"] = msg.task_id;
//     j["status"] = static_cast<int>(msg.status);
//     j["progress_percent"] = msg.progress_percent;
//     j["current_action"] = msg.current_action;
//     j["message"] = msg.message;
//     j["output_vars"] = msg.output_vars;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeTaskProgress(const std::vector<uint8_t>& data, TaskProgressMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.segment_id = j["segment_id"].get<std::string>();
//         msg.task_id = j["task_id"].get<std::string>();
//         msg.status = static_cast<TaskStatus>(j["status"].get<int>());
//         msg.progress_percent = j["progress_percent"].get<uint8_t>();
//         msg.current_action = j["current_action"].get<std::string>();
//         msg.message = j["message"].get<std::string>();
//         msg.output_vars = j["output_vars"].get<std::map<std::string, std::string>>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeTaskComplete(const TaskCompleteMessage& msg) {
//     json j;
//     j["segment_id"] = msg.segment_id;
//     j["task_id"] = msg.task_id;
//     j["success"] = msg.success;
//     j["actual_start"] = msg.actual_start;
//     j["actual_end"] = msg.actual_end;
//     j["actual_profit"] = msg.actual_profit;
//     j["result_summary"] = msg.result_summary;
//     j["output_data"] = msg.output_data;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeTaskComplete(const std::vector<uint8_t>& data, TaskCompleteMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.segment_id = j["segment_id"].get<std::string>();
//         msg.task_id = j["task_id"].get<std::string>();
//         msg.success = j["success"].get<bool>();
//         msg.actual_start = j["actual_start"].get<std::string>();
//         msg.actual_end = j["actual_end"].get<std::string>();
//         msg.actual_profit = j["actual_profit"].get<int>();
//         msg.result_summary = j["result_summary"].get<std::string>();
//         msg.output_data = j["output_data"].get<std::map<std::string, std::string>>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeSemAcquireRequest(const SemaphoreAcquireRequest& msg) {
//     json j;
//     j["request_id"] = msg.request_id;
//     j["node_id"] = msg.node_id;
//     j["semaphore_id"] = msg.semaphore_id;
//     j["task_id"] = msg.task_id;
//     j["priority"] = static_cast<int>(msg.priority);
//     j["timeout_ms"] = msg.timeout_ms;
//     j["blocking"] = msg.blocking;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeSemAcquireRequest(const std::vector<uint8_t>& data, SemaphoreAcquireRequest& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.request_id = j["request_id"].get<std::string>();
//         msg.node_id = j["node_id"].get<std::string>();
//         msg.semaphore_id = j["semaphore_id"].get<std::string>();
//         msg.task_id = j["task_id"].get<std::string>();
//         msg.priority = static_cast<Priority>(j["priority"].get<int>());
//         msg.timeout_ms = j["timeout_ms"].get<int>();
//         msg.blocking = j["blocking"].get<bool>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeSemAcquireResponse(const SemaphoreAcquireResponse& msg) {
//     json j;
//     j["request_id"] = msg.request_id;
//     j["semaphore_id"] = msg.semaphore_id;
//     j["granted"] = msg.granted;
//     j["queued"] = msg.queued;
//     j["queue_position"] = msg.queue_position;
//     j["estimated_wait_ms"] = msg.estimated_wait_ms;
//     j["grant_token"] = msg.grant_token;
//     j["deny_reason"] = msg.deny_reason;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeSemAcquireResponse(const std::vector<uint8_t>& data, SemaphoreAcquireResponse& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.request_id = j["request_id"].get<std::string>();
//         msg.semaphore_id = j["semaphore_id"].get<std::string>();
//         msg.granted = j["granted"].get<bool>();
//         msg.queued = j["queued"].get<bool>();
//         msg.queue_position = j["queue_position"].get<int>();
//         msg.estimated_wait_ms = j["estimated_wait_ms"].get<int>();
//         msg.grant_token = j["grant_token"].get<uint64_t>();
//         msg.deny_reason = j["deny_reason"].get<std::string>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeSemRelease(const SemaphoreReleaseMessage& msg) {
//     json j;
//     j["node_id"] = msg.node_id;
//     j["semaphore_id"] = msg.semaphore_id;
//     j["grant_token"] = msg.grant_token;
//     j["task_id"] = msg.task_id;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeSemRelease(const std::vector<uint8_t>& data, SemaphoreReleaseMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.node_id = j["node_id"].get<std::string>();
//         msg.semaphore_id = j["semaphore_id"].get<std::string>();
//         msg.grant_token = j["grant_token"].get<uint64_t>();
//         msg.task_id = j["task_id"].get<std::string>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeSemReleaseAck(const SemaphoreReleaseAck& msg) {
//     json j;
//     j["semaphore_id"] = msg.semaphore_id;
//     j["success"] = msg.success;
//     j["available_permits"] = msg.available_permits;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeSemReleaseAck(const std::vector<uint8_t>& data, SemaphoreReleaseAck& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.semaphore_id = j["semaphore_id"].get<std::string>();
//         msg.success = j["success"].get<bool>();
//         msg.available_permits = j["available_permits"].get<int>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeStateSyncRequest(const StateSyncRequest& msg) {
//     json j;
//     j["requester_id"] = msg.requester_id;
//     j["sync_items"] = msg.sync_items;
//     j["full_sync"] = msg.full_sync;
//     j["last_sync_time_ms"] = msg.last_sync_time_ms;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeStateSyncRequest(const std::vector<uint8_t>& data, StateSyncRequest& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.requester_id = j["requester_id"].get<std::string>();
//         msg.sync_items = j["sync_items"].get<std::vector<std::string>>();
//         msg.full_sync = j["full_sync"].get<bool>();
//         msg.last_sync_time_ms = j["last_sync_time_ms"].get<uint64_t>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeStateSyncResponse(const StateSyncResponse& msg) {
//     json j;
//     j["responder_id"] = msg.responder_id;
    
//     json nodes = json::array();
//     for (const auto& node : msg.node_states) {
//         json node_j;
//         node_j["node_id"] = node.node_id;
//         node_j["status"] = static_cast<int>(node.status);
//         node_j["battery_percent"] = node.battery_percent;
//         node_j["storage_available_mb"] = node.storage_available_mb;
//         node_j["payload_status"] = node.payload_status;
//         node_j["active_tasks"] = node.active_tasks;
//         node_j["last_update_ms"] = node.last_update_ms;
//         nodes.push_back(node_j);
//     }
//     j["node_states"] = nodes;
    
//     j["semaphore_status"] = msg.semaphore_status;
//     j["sync_time_ms"] = msg.sync_time_ms;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeStateSyncResponse(const std::vector<uint8_t>& data, StateSyncResponse& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.responder_id = j["responder_id"].get<std::string>();
        
//         msg.node_states.clear();
//         for (const auto& node_j : j["node_states"]) {
//             NodeStateData node;
//             node.node_id = node_j["node_id"].get<std::string>();
//             node.status = static_cast<NodeStatus>(node_j["status"].get<int>());
//             node.battery_percent = node_j["battery_percent"].get<int>();
//             node.storage_available_mb = node_j["storage_available_mb"].get<int>();
//             node.payload_status = node_j["payload_status"].get<std::string>();
//             node.active_tasks = node_j["active_tasks"].get<std::vector<std::string>>();
//             node.last_update_ms = node_j["last_update_ms"].get<uint64_t>();
//             msg.node_states.push_back(node);
//         }
        
//         msg.semaphore_status = j["semaphore_status"].get<std::map<std::string, int>>();
//         msg.sync_time_ms = j["sync_time_ms"].get<uint64_t>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeBarrierWait(const BarrierWaitMessage& msg) {
//     json j;
//     j["sync_id"] = msg.sync_id;
//     j["node_id"] = msg.node_id;
//     j["barrier_type"] = msg.barrier_type;
//     j["current_status"] = static_cast<int>(msg.current_status);
//     j["arrival_time_ms"] = msg.arrival_time_ms;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeBarrierWait(const std::vector<uint8_t>& data, BarrierWaitMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.sync_id = j["sync_id"].get<std::string>();
//         msg.node_id = j["node_id"].get<std::string>();
//         msg.barrier_type = j["barrier_type"].get<std::string>();
//         msg.current_status = static_cast<NodeStatus>(j["current_status"].get<int>());
//         msg.arrival_time_ms = j["arrival_time_ms"].get<uint64_t>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeBarrierRelease(const BarrierReleaseMessage& msg) {
//     json j;
//     j["sync_id"] = msg.sync_id;
//     j["participants"] = msg.participants;
//     j["missing_nodes"] = msg.missing_nodes;
//     j["all_arrived"] = msg.all_arrived;
//     j["release_reason"] = msg.release_reason;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeBarrierRelease(const std::vector<uint8_t>& data, BarrierReleaseMessage& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.sync_id = j["sync_id"].get<std::string>();
//         msg.participants = j["participants"].get<std::vector<std::string>>();
//         msg.missing_nodes = j["missing_nodes"].get<std::vector<std::string>>();
//         msg.all_arrived = j["all_arrived"].get<bool>();
//         msg.release_reason = j["release_reason"].get<std::string>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::vector<uint8_t> MessageSerializer::serializeError(const ErrorResponse& msg) {
//     json j;
//     j["error_code"] = static_cast<int>(msg.error_code);
//     j["error_message"] = msg.error_message;
//     j["original_msg_type"] = msg.original_msg_type;
//     j["original_seq_id"] = msg.original_seq_id;
//     j["details"] = msg.details;
    
//     std::string str = j.dump();
//     return std::vector<uint8_t>(str.begin(), str.end());
// }

// bool MessageSerializer::deserializeError(const std::vector<uint8_t>& data, ErrorResponse& msg) {
//     try {
//         std::string str(data.begin(), data.end());
//         json j = json::parse(str);
        
//         msg.error_code = static_cast<ErrorCode>(j["error_code"].get<int>());
//         msg.error_message = j["error_message"].get<std::string>();
//         msg.original_msg_type = j["original_msg_type"].get<std::string>();
//         msg.original_seq_id = j["original_seq_id"].get<uint32_t>();
//         msg.details = j["details"].get<std::map<std::string, std::string>>();
        
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// uint32_t MessageSerializer::calculateChecksum(const std::vector<uint8_t>& data) {
//     uint32_t checksum = 0;
//     for (uint8_t byte : data) {
//         checksum += byte;
//     }
//     return checksum;
// }

// // ============================================================================
// // InterSatComm 实现
// // ============================================================================

// InterSatComm::InterSatComm(const CommConfig& config)
//     : config_(config)
//     , running_(false)
//     , initialized_(false)
//     , send_queue_(config.message_queue_size)
//     , recv_queue_(config.message_queue_size)
//     , sequence_id_(0)
//     , start_time_ms_(0)
//     , listen_socket_(INVALID_SOCKET_VALUE)
//     , local_status_(NodeStatus::INITIALIZING) {
    
//     memset(&stats_, 0, sizeof(stats_));
// }

// InterSatComm::~InterSatComm() {
//     stop();
// }

// bool InterSatComm::initialize() {
//     if (initialized_.load()) {
//         return true;
//     }
    
// #ifdef _WIN32
//     WSADATA wsaData;
//     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//         return false;
//     }
// #endif
    
//     if (!initializeSocket()) {
//         return false;
//     }
    
//     initialized_.store(true);
//     return true;
// }

// bool InterSatComm::start() {
//     if (running_.load()) {
//         return true;
//     }
    
//     if (!initialized_.load()) {
//         if (!initialize()) {
//             return false;
//         }
//     }
    
//     running_.store(true);
//     start_time_ms_ = getCurrentTimeMs();
    
//     // 启动工作线程
//     accept_thread_ = std::unique_ptr<std::thread>(
//         new std::thread(&InterSatComm::acceptThreadFunc, this));
//     receive_thread_ = std::unique_ptr<std::thread>(
//         new std::thread(&InterSatComm::receiveThreadFunc, this));
//     send_thread_ = std::unique_ptr<std::thread>(
//         new std::thread(&InterSatComm::sendThreadFunc, this));
//     heartbeat_thread_ = std::unique_ptr<std::thread>(
//         new std::thread(&InterSatComm::heartbeatThreadFunc, this));
    
//     return true;
// }

// void InterSatComm::stop() {
//     if (!running_.load()) {
//         return;
//     }
    
//     running_.store(false);
    
//     // 等待线程结束
//     if (accept_thread_ && accept_thread_->joinable()) {
//         accept_thread_->join();
//     }
//     if (receive_thread_ && receive_thread_->joinable()) {
//         receive_thread_->join();
//     }
//     if (send_thread_ && send_thread_->joinable()) {
//         send_thread_->join();
//     }
//     if (heartbeat_thread_ && heartbeat_thread_->joinable()) {
//         heartbeat_thread_->join();
//     }
    
//     // 清理资源
//     cleanupSocket();
    
//     {
//         std::lock_guard<std::mutex> lock(nodes_mutex_);
//         nodes_.clear();
//     }
    
//     send_queue_.clear();
//     recv_queue_.clear();
    
// #ifdef _WIN32
//     WSACleanup();
// #endif
    
//     initialized_.store(false);
// }

// void InterSatComm::registerHandler(IMessageHandler* handler) {
//     if (handler) {
//         std::lock_guard<std::mutex> lock(handlers_mutex_);
//         handlers_.push_back(handler);
//     }
// }

// void InterSatComm::unregisterHandler(IMessageHandler* handler) {
//     std::lock_guard<std::mutex> lock(handlers_mutex_);
//     handlers_.erase(
//         std::remove(handlers_.begin(), handlers_.end(), handler),
//         handlers_.end());
// }

// void InterSatComm::registerCallback(MessageType type, MessageCallback callback) {
//     std::lock_guard<std::mutex> lock(callbacks_mutex_);
//     callbacks_[type].push_back(callback);
// }

// bool InterSatComm::connectToNode(const std::string& ip_address, uint16_t port) {
//     RemoteNode node;
//     node.ip_address = ip_address;
//     node.port = port;
//     node.status = NodeStatus::INITIALIZING;
//     node.connect_time_ms = getCurrentTimeMs();
    
//     if (!connectSocket(ip_address, port, node.socket_fd)) {
//         return false;
//     }
    
//     // 发送注册消息
//     NodeRegisterMessage reg_msg;
//     reg_msg.node_id = config_.node_id;
//     reg_msg.node_name = config_.node_name;
//     reg_msg.node_type = config_.node_type;
//     reg_msg.ip_address = config_.bind_address;
//     reg_msg.port = config_.bind_port;
    
//     auto payload = MessageSerializer::serializeNodeRegister(reg_msg);
//     Message msg = buildMessage(MessageType::NODE_REGISTER, "", payload);
    
//     auto data = MessageSerializer::serialize(msg);
//     if (!sendData(node.socket_fd, data)) {
//         closesocket(node.socket_fd);
//         return false;
//     }
    
//     addNode(node);
//     return true;
// }

// void InterSatComm::disconnectNode(const std::string& node_id) {
//     removeNode(node_id);
// }

// bool InterSatComm::registerToCoordinator(const std::string& coordinator_addr, uint16_t port) {
//     return connectToNode(coordinator_addr, port);
// }

// bool InterSatComm::getNodeInfo(const std::string& node_id, RemoteNode& info) const {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
//     auto it = nodes_.find(node_id);
//     if (it != nodes_.end()) {
//         info = it->second;
//         return true;
//     }
//     return false;
// }

// std::vector<std::string> InterSatComm::getConnectedNodes() const {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
//     std::vector<std::string> result;
//     for (const auto& pair : nodes_) {
//         result.push_back(pair.first);
//     }
//     return result;
// }

// NodeStatus InterSatComm::getNodeStatus(const std::string& node_id) const {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
//     auto it = nodes_.find(node_id);
//     if (it != nodes_.end()) {
//         return it->second.status;
//     }
//     return NodeStatus::UNKNOWN;
// }

// void InterSatComm::updateLocalStatus(NodeStatus status) {
//     std::lock_guard<std::mutex> lock(local_status_mutex_);
//     local_status_ = status;
// }

// bool InterSatComm::sendMessage(const std::string& dest_node_id, const Message& message) {
//     return send_queue_.push(message);
// }

// bool InterSatComm::sendMessageWithRetry(const std::string& dest_node_id, 
//                                         const Message& message, 
//                                         int retry_count) {
//     if (retry_count < 0) {
//         retry_count = config_.retry_count;
//     }
    
//     for (int i = 0; i <= retry_count; ++i) {
//         if (sendMessage(dest_node_id, message)) {
//             return true;
//         }
        
//         if (i < retry_count) {
//             std::this_thread::sleep_for(
//                 std::chrono::milliseconds(config_.retry_interval_ms));
//         }
//     }
    
//     return false;
// }

// bool InterSatComm::broadcastMessage(const Message& message) {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
    
//     bool all_sent = true;
//     for (const auto& pair : nodes_) {
//         Message msg = message;
//         msg.header.dest_node_id = pair.first;
//         if (!send_queue_.push(msg)) {
//             all_sent = false;
//         }
//     }
    
//     return all_sent;
// }

// bool InterSatComm::sendAndWaitResponse(const std::string& dest_node_id, 
//                                        const Message& request,
//                                        Message& response, 
//                                        uint32_t timeout_ms) {
//     auto promise = std::make_shared<std::promise<Message>>();
//     auto future = promise->get_future();
    
//     {
//         std::lock_guard<std::mutex> lock(pending_mutex_);
//         PendingRequest pending;
//         pending.request = request;
//         pending.response_promise = promise;
//         pending.send_time_ms = getCurrentTimeMs();
//         pending.timeout_ms = timeout_ms;
//         pending_requests_[request.header.sequence_id] = pending;
//     }
    
//     if (!sendMessage(dest_node_id, request)) {
//         std::lock_guard<std::mutex> lock(pending_mutex_);
//         pending_requests_.erase(request.header.sequence_id);
//         return false;
//     }
    
//     auto status = future.wait_for(std::chrono::milliseconds(timeout_ms));
    
//     if (status == std::future_status::timeout) {
//         std::lock_guard<std::mutex> lock(pending_mutex_);
//         pending_requests_.erase(request.header.sequence_id);
//         return false;
//     }
    
//     response = future.get();
    
//     std::lock_guard<std::mutex> lock(pending_mutex_);
//     pending_requests_.erase(request.header.sequence_id);
    
//     return true;
// }

// bool InterSatComm::sendHeartbeat(const std::string& dest_node_id) {
//     HeartbeatMessage hb;
//     hb.node_id = config_.node_id;
//     hb.uptime_ms = getCurrentTimeMs() - start_time_ms_;
//     hb.status = local_status_;
//     hb.cpu_usage_percent = 0;
//     hb.memory_usage_percent = 0;
//     hb.battery_percent = 100;
//     hb.active_tasks = 0;
//     hb.pending_messages = static_cast<uint32_t>(send_queue_.size());
    
//     auto payload = MessageSerializer::serializeHeartbeat(hb);
//     Message msg = buildMessage(MessageType::HEARTBEAT, dest_node_id, payload);
    
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendTaskAssign(const std::string& dest_node_id, 
//                                   const TaskAssignMessage& task) {
//     auto payload = MessageSerializer::serializeTaskAssign(task);
//     Message msg = buildMessage(MessageType::TASK_ASSIGN, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendTaskProgress(const std::string& dest_node_id, 
//                                     const TaskProgressMessage& progress) {
//     auto payload = MessageSerializer::serializeTaskProgress(progress);
//     Message msg = buildMessage(MessageType::TASK_PROGRESS, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendTaskComplete(const std::string& dest_node_id, 
//                                     const TaskCompleteMessage& complete) {
//     auto payload = MessageSerializer::serializeTaskComplete(complete);
//     Message msg = buildMessage(MessageType::TASK_COMPLETE, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendSemaphoreRequest(const std::string& dest_node_id, 
//                                        const SemaphoreAcquireRequest& request) {
//     auto payload = MessageSerializer::serializeSemAcquireRequest(request);
//     Message msg = buildMessage(MessageType::SEM_ACQUIRE_REQUEST, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendSemaphoreRelease(const std::string& dest_node_id, 
//                                        const SemaphoreReleaseMessage& release) {
//     auto payload = MessageSerializer::serializeSemRelease(release);
//     Message msg = buildMessage(MessageType::SEM_RELEASE, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendStateSyncRequest(const std::string& dest_node_id, 
//                                        const StateSyncRequest& request) {
//     auto payload = MessageSerializer::serializeStateSyncRequest(request);
//     Message msg = buildMessage(MessageType::STATE_SYNC_REQUEST, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// bool InterSatComm::sendBarrierWait(const std::string& dest_node_id, 
//                                   const BarrierWaitMessage& barrier) {
//     auto payload = MessageSerializer::serializeBarrierWait(barrier);
//     Message msg = buildMessage(MessageType::BARRIER_WAIT, dest_node_id, payload);
//     return sendMessage(dest_node_id, msg);
// }

// InterSatComm::Statistics InterSatComm::getStatistics() const {
//     std::lock_guard<std::mutex> lock(stats_mutex_);
//     Statistics result = stats_;
//     result.uptime_ms = getCurrentTimeMs() - start_time_ms_;
//     return result;
// }

// void InterSatComm::resetStatistics() {
//     std::lock_guard<std::mutex> lock(stats_mutex_);
//     memset(&stats_, 0, sizeof(stats_));
// }

// void InterSatComm::updateConfig(const CommConfig& config) {
//     config_ = config;
// }

// // ========================================================================
// // 内部方法实现
// // ========================================================================

// void InterSatComm::acceptThreadFunc() {
//     while (running_.load()) {
//         RemoteNode node;
//         if (acceptConnection(node)) {
//             // 处理新连接
//         }
//         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     }
// }

// void InterSatComm::receiveThreadFunc() {
//     while (running_.load()) {
//         Message message;
//         if (recv_queue_.pop(message, 100)) {
//             processReceivedMessage(message);
//         }
//     }
// }

// void InterSatComm::sendThreadFunc() {
//     while (running_.load()) {
//         Message message;
//         if (send_queue_.pop(message, 100)) {
//             std::lock_guard<std::mutex> lock(nodes_mutex_);
//             auto it = nodes_.find(message.header.dest_node_id);
//             if (it != nodes_.end()) {
//                 auto data = MessageSerializer::serialize(message);
//                 if (sendData(it->second.socket_fd, data)) {
//                     std::lock_guard<std::mutex> stats_lock(stats_mutex_);
//                     stats_.total_messages_sent++;
//                     stats_.total_bytes_sent += data.size();
//                     it->second.messages_sent++;
//                     it->second.bytes_sent += data.size();
//                 } else {
//                     std::lock_guard<std::mutex> stats_lock(stats_mutex_);
//                     stats_.failed_sends++;
//                 }
//             }
//         }
//     }
// }

// void InterSatComm::heartbeatThreadFunc() {
//     while (running_.load()) {
//         std::this_thread::sleep_for(
//             std::chrono::milliseconds(config_.heartbeat_interval_ms));
        
//         if (!running_.load()) break;
        
//         // 发送心跳
//         auto nodes = getConnectedNodes();
//         for (const auto& node_id : nodes) {
//             sendHeartbeat(node_id);
//         }
        
//         // 检查超时
//         checkNodeTimeouts();
//     }
// }

// void InterSatComm::processReceivedMessage(const Message& message) {
//     // 通知处理器
//     {
//         std::lock_guard<std::mutex> lock(handlers_mutex_);
//         for (auto handler : handlers_) {
//             handler->onMessageReceived(message);
//         }
//     }
    
//     // 调用回调
//     {
//         std::lock_guard<std::mutex> lock(callbacks_mutex_);
//         auto it = callbacks_.find(message.header.msg_type);
//         if (it != callbacks_.end()) {
//             for (auto& callback : it->second) {
//                 callback(message);
//             }
//         }
//     }
    
//     // 处理响应
//     {
//         std::lock_guard<std::mutex> lock(pending_mutex_);
//         auto it = pending_requests_.find(message.header.sequence_id);
//         if (it != pending_requests_.end()) {
//             it->second.response_promise->set_value(message);
//         }
//     }
    
//     // 特定消息类型处理
//     switch (message.header.msg_type) {
//         case MessageType::HEARTBEAT:
//             handleHeartbeat(message);
//             break;
//         case MessageType::NODE_REGISTER:
//         case MessageType::NODE_REGISTER_ACK:
//             handleNodeRegister(message);
//             break;
//         default:
//             break;
//     }
// }

// void InterSatComm::handleHeartbeat(const Message& message) {
//     HeartbeatMessage hb;
//     if (MessageSerializer::deserializeHeartbeat(message.payload, hb)) {
//         updateNodeHeartbeat(hb.node_id);
        
//         std::lock_guard<std::mutex> lock(handlers_mutex_);
//         for (auto handler : handlers_) {
//             handler->onHeartbeatReceived(hb.node_id, hb);
//         }
//     }
// }

// void InterSatComm::handleNodeRegister(const Message& message) {
//     // 实现节点注册逻辑
// }

// void InterSatComm::handleTaskMessage(const Message& message) {
//     // 实现任务消息处理
// }

// void InterSatComm::handleSemaphoreMessage(const Message& message) {
//     // 实现信号量消息处理
// }

// void InterSatComm::handleStateSyncMessage(const Message& message) {
//     // 实现状态同步处理
// }

// void InterSatComm::addNode(const RemoteNode& node) {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
//     nodes_[node.node_id] = node;
    
//     std::lock_guard<std::mutex> stats_lock(stats_mutex_);
//     stats_.connected_nodes = static_cast<uint32_t>(nodes_.size());
// }

// void InterSatComm::removeNode(const std::string& node_id) {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
//     auto it = nodes_.find(node_id);
//     if (it != nodes_.end()) {
//         closesocket(it->second.socket_fd);
//         nodes_.erase(it);
        
//         std::lock_guard<std::mutex> stats_lock(stats_mutex_);
//         stats_.connected_nodes = static_cast<uint32_t>(nodes_.size());
//     }
// }

// void InterSatComm::updateNodeHeartbeat(const std::string& node_id) {
//     std::lock_guard<std::mutex> lock(nodes_mutex_);
//     auto it = nodes_.find(node_id);
//     if (it != nodes_.end()) {
//         it->second.last_heartbeat_time_ms = getCurrentTimeMs();
//         it->second.missed_heartbeats = 0;
//     }
// }

// void InterSatComm::checkNodeTimeouts() {
//     uint64_t now = getCurrentTimeMs();
//     std::vector<std::string> timed_out_nodes;
    
//     {
//         std::lock_guard<std::mutex> lock(nodes_mutex_);
//         for (auto& pair : nodes_) {
//             if (now - pair.second.last_heartbeat_time_ms > config_.heartbeat_timeout_ms) {
//                 pair.second.missed_heartbeats++;
//                 if (pair.second.missed_heartbeats >= config_.max_missed_heartbeats) {
//                     timed_out_nodes.push_back(pair.first);
//                 }
//             }
//         }
//     }
    
//     // 通知超时节点
//     for (const auto& node_id : timed_out_nodes) {
//         std::lock_guard<std::mutex> lock(handlers_mutex_);
//         for (auto handler : handlers_) {
//             handler->onHeartbeatTimeout(node_id);
//         }
//         removeNode(node_id);
//     }
// }

// Message InterSatComm::buildMessage(MessageType type, 
//                                    const std::string& dest_node_id,
//                                    const std::vector<uint8_t>& payload) {
//     Message msg;
//     msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     msg.header.version = MessageHeader::PROTOCOL_VERSION;
//     msg.header.msg_type = type;
//     msg.header.sequence_id = getNextSequenceId();
//     msg.header.timestamp_ms = getCurrentTimeMs();
//     msg.header.source_node_id = config_.node_id;
//     msg.header.dest_node_id = dest_node_id;
//     msg.header.priority = Priority::NORMAL;
//     msg.header.payload_size = static_cast<uint32_t>(payload.size());
//     msg.payload = payload;
//     msg.header.checksum = MessageSerializer::calculateChecksum(payload);
    
//     return msg;
// }

// uint32_t InterSatComm::getNextSequenceId() {
//     return sequence_id_.fetch_add(1);
// }

// uint64_t InterSatComm::getCurrentTimeMs() const {
//     return std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()).count();
// }

// bool InterSatComm::initializeSocket() {
//     listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     if (listen_socket_ == INVALID_SOCKET_VALUE) {
//         return false;
//     }
    
//     // 设置地址重用
//     int opt = 1;
//     setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, 
//                reinterpret_cast<char*>(&opt), sizeof(opt));
    
//     return bindSocket();
// }

// void InterSatComm::cleanupSocket() {
//     if (listen_socket_ != INVALID_SOCKET_VALUE) {
//         closesocket(listen_socket_);
//         listen_socket_ = INVALID_SOCKET_VALUE;
//     }
// }

// bool InterSatComm::bindSocket() {
//     sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(config_.bind_port);
//     addr.sin_addr.s_addr = INADDR_ANY;
    
//     if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
//         return false;
//     }
    
//     if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
//         return false;
//     }
    
//     return true;
// }

// bool InterSatComm::acceptConnection(RemoteNode& node) {
//     sockaddr_in client_addr;
//     socklen_t addr_len = sizeof(client_addr);
    
//     socket_t client_socket = accept(listen_socket_, 
//                                reinterpret_cast<sockaddr*>(&client_addr), 
//                                &addr_len);
    
//     if (client_socket == INVALID_SOCKET_VALUE) {
//         return false;
//     }
    
//     node.socket_fd = client_socket;
//     node.ip_address = inet_ntoa(client_addr.sin_addr);
//     node.port = ntohs(client_addr.sin_port);
//     node.connect_time_ms = getCurrentTimeMs();
//     node.status = NodeStatus::INITIALIZING;
    
//     return true;
// }

// bool InterSatComm::connectSocket(const std::string& ip, uint16_t port, socket_t& socket_fd) {
//     socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//     if (socket_fd == INVALID_SOCKET_VALUE) {
//         return false;
//     }
    
//     sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(port);
//     addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
//     if (connect(socket_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
//         closesocket(socket_fd);
//         socket_fd = INVALID_SOCKET_VALUE;
//         return false;
//     }
    
//     return true;
// }

// bool InterSatComm::sendData(socket_t socket_fd, const std::vector<uint8_t>& data) {
//     size_t total_sent = 0;
//     while (total_sent < data.size()) {
//         int sent = send(socket_fd, 
//                        reinterpret_cast<const char*>(data.data() + total_sent),
//                        static_cast<int>(data.size() - total_sent), 
//                        0);
        
//         if (sent == SOCKET_ERROR) {
//             return false;
//         }
        
//         total_sent += sent;
//     }
    
//     return true;
// }

// bool InterSatComm::receiveData(socket_t socket_fd, std::vector<uint8_t>& data) {
//     uint8_t buffer[4096];
//     int received = recv(socket_fd, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
    
//     if (received <= 0) {
//         return false;
//     }
    
//     data.assign(buffer, buffer + received);
//     return true;
// }

// } // namespace coordinator
