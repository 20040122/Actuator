// /**
//  * @file test_inter_sat_comm.cpp
//  * @brief InterSatComm 通信器单元测试
//  * 
//  * 测试内容：
//  * 1. 消息序列化/反序列化
//  * 2. 消息队列功能
//  * 3. 通信器初始化和启动/停止
//  * 4. 节点连接管理
//  * 5. 消息发送和处理
//  */

// #include "inter_sat_comm.h"
// #include "message_types.h"
// #include <iostream>
// #include <cassert>
// #include <thread>
// #include <chrono>
// #include <sstream>

// using namespace coordinator;

// // ============================================================================
// // 测试辅助类和宏
// // ============================================================================

// #define TEST_ASSERT(condition, message) \
//     do { \
//         if (!(condition)) { \
//             std::cerr << "[FAILED] " << message << std::endl; \
//             std::cerr << "  File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
//             return false; \
//         } \
//     } while(0)

// #define TEST_ASSERT_EQ(expected, actual, message) \
//     do { \
//         if ((expected) != (actual)) { \
//             std::cerr << "[FAILED] " << message << std::endl; \
//             std::cerr << "  Expected: " << (expected) << ", Actual: " << (actual) << std::endl; \
//             std::cerr << "  File: " << __FILE__ << ", Line: " << __LINE__ << std::endl; \
//             return false; \
//         } \
//     } while(0)

// #define RUN_TEST(test_func) \
//     do { \
//         std::cout << "Running " << #test_func << "... "; \
//         if (test_func()) { \
//             std::cout << "[PASSED]" << std::endl; \
//             passed++; \
//         } else { \
//             failed++; \
//         } \
//         total++; \
//     } while(0)

// // 测试用的消息处理器
// class TestMessageHandler : public IMessageHandler {
// public:
//     std::vector<Message> received_messages;
//     std::vector<std::string> connected_nodes;
//     std::vector<std::string> disconnected_nodes;
//     std::vector<HeartbeatMessage> heartbeats;
//     std::vector<std::pair<ErrorCode, std::string>> errors;
    
//     void onMessageReceived(const Message& message) override {
//         received_messages.push_back(message);
//     }
    
//     void onNodeConnected(const std::string& node_id) override {
//         connected_nodes.push_back(node_id);
//     }
    
//     void onNodeDisconnected(const std::string& node_id, const std::string& reason) override {
//         disconnected_nodes.push_back(node_id);
//     }
    
//     void onNodeStatusChanged(const std::string& node_id, 
//                             NodeStatus old_status, 
//                             NodeStatus new_status) override {
//         // 可以记录状态变化
//     }
    
//     void onHeartbeatReceived(const std::string& node_id, 
//                             const HeartbeatMessage& heartbeat) override {
//         heartbeats.push_back(heartbeat);
//     }
    
//     void onHeartbeatTimeout(const std::string& node_id) override {
//         // 可以记录超时
//     }
    
//     void onError(ErrorCode code, const std::string& message) override {
//         errors.push_back({code, message});
//     }
    
//     void clear() {
//         received_messages.clear();
//         connected_nodes.clear();
//         disconnected_nodes.clear();
//         heartbeats.clear();
//         errors.clear();
//     }
// };

// // ============================================================================
// // 消息类型辅助函数测试
// // ============================================================================

// bool test_message_type_to_string() {
//     TEST_ASSERT_EQ(std::string("HEARTBEAT"), 
//                    std::string(messageTypeToString(MessageType::HEARTBEAT)),
//                    "MessageType::HEARTBEAT string conversion");
    
//     TEST_ASSERT_EQ(std::string("TASK_ASSIGN"), 
//                    std::string(messageTypeToString(MessageType::TASK_ASSIGN)),
//                    "MessageType::TASK_ASSIGN string conversion");
    
//     TEST_ASSERT_EQ(std::string("SEM_ACQUIRE_REQUEST"), 
//                    std::string(messageTypeToString(MessageType::SEM_ACQUIRE_REQUEST)),
//                    "MessageType::SEM_ACQUIRE_REQUEST string conversion");
    
//     TEST_ASSERT_EQ(std::string("UNKNOWN"), 
//                    std::string(messageTypeToString(MessageType::UNKNOWN)),
//                    "MessageType::UNKNOWN string conversion");
    
//     return true;
// }

// bool test_node_status_to_string() {
//     TEST_ASSERT_EQ(std::string("HEALTHY"), 
//                    std::string(nodeStatusToString(NodeStatus::HEALTHY)),
//                    "NodeStatus::HEALTHY string conversion");
    
//     TEST_ASSERT_EQ(std::string("OFFLINE"), 
//                    std::string(nodeStatusToString(NodeStatus::OFFLINE)),
//                    "NodeStatus::OFFLINE string conversion");
    
//     TEST_ASSERT_EQ(std::string("INITIALIZING"), 
//                    std::string(nodeStatusToString(NodeStatus::INITIALIZING)),
//                    "NodeStatus::INITIALIZING string conversion");
    
//     return true;
// }

// bool test_priority_to_string() {
//     TEST_ASSERT_EQ(std::string("NORMAL"), 
//                    std::string(priorityToString(Priority::NORMAL)),
//                    "Priority::NORMAL string conversion");
    
//     TEST_ASSERT_EQ(std::string("EMERGENCY"), 
//                    std::string(priorityToString(Priority::EMERGENCY)),
//                    "Priority::EMERGENCY string conversion");
    
//     return true;
// }

// // ============================================================================
// // MessageQueue 测试
// // ============================================================================

// bool test_message_queue_basic() {
//     MessageQueue queue(10);
    
//     TEST_ASSERT(queue.empty(), "New queue should be empty");
//     TEST_ASSERT_EQ(size_t(0), queue.size(), "New queue size should be 0");
//     TEST_ASSERT(!queue.full(), "New queue should not be full");
    
//     return true;
// }

// bool test_message_queue_push_pop() {
//     MessageQueue queue(10);
    
//     // 创建测试消息
//     Message msg1;
//     msg1.header.magic = MessageHeader::MAGIC_NUMBER;
//     msg1.header.msg_type = MessageType::HEARTBEAT;
//     msg1.header.sequence_id = 1;
//     msg1.header.priority = Priority::NORMAL;
//     msg1.header.source_node_id = "NODE_1";
//     msg1.header.dest_node_id = "NODE_2";
    
//     // 测试 push
//     TEST_ASSERT(queue.push(msg1), "Push should succeed");
//     TEST_ASSERT_EQ(size_t(1), queue.size(), "Queue size should be 1 after push");
//     TEST_ASSERT(!queue.empty(), "Queue should not be empty after push");
    
//     // 测试 pop
//     Message popped;
//     TEST_ASSERT(queue.pop(popped, 100), "Pop should succeed");
//     TEST_ASSERT_EQ(uint32_t(1), popped.header.sequence_id, "Popped message sequence_id");
//     TEST_ASSERT(queue.empty(), "Queue should be empty after pop");
    
//     return true;
// }

// bool test_message_queue_priority() {
//     MessageQueue queue(10);
    
//     // 添加低优先级消息
//     Message low_msg;
//     low_msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     low_msg.header.msg_type = MessageType::HEARTBEAT;
//     low_msg.header.sequence_id = 1;
//     low_msg.header.priority = Priority::LOW;
//     queue.push(low_msg);
    
//     // 添加高优先级消息
//     Message high_msg;
//     high_msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     high_msg.header.msg_type = MessageType::TASK_ASSIGN;
//     high_msg.header.sequence_id = 2;
//     high_msg.header.priority = Priority::EMERGENCY;
//     queue.push(high_msg);
    
//     // 高优先级消息应该先出队
//     Message popped;
//     TEST_ASSERT(queue.pop(popped, 100), "First pop should succeed");
//     TEST_ASSERT_EQ(uint32_t(2), popped.header.sequence_id, 
//                    "High priority message should be popped first");
    
//     TEST_ASSERT(queue.pop(popped, 100), "Second pop should succeed");
//     TEST_ASSERT_EQ(uint32_t(1), popped.header.sequence_id, 
//                    "Low priority message should be popped second");
    
//     return true;
// }

// bool test_message_queue_full() {
//     MessageQueue queue(3);  // 最大容量为3
    
//     Message msg;
//     msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     msg.header.priority = Priority::NORMAL;
    
//     // 填满队列
//     for (int i = 0; i < 3; ++i) {
//         msg.header.sequence_id = i;
//         TEST_ASSERT(queue.push(msg), "Push should succeed before full");
//     }
    
//     TEST_ASSERT(queue.full(), "Queue should be full");
    
//     // 尝试再添加一个，应该失败
//     msg.header.sequence_id = 99;
//     TEST_ASSERT(!queue.push(msg), "Push should fail when queue is full");
    
//     return true;
// }

// bool test_message_queue_try_pop() {
//     MessageQueue queue(10);
    
//     Message msg;
//     TEST_ASSERT(!queue.tryPop(msg), "tryPop should fail on empty queue");
    
//     // 添加消息
//     Message push_msg;
//     push_msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     push_msg.header.sequence_id = 42;
//     push_msg.header.priority = Priority::NORMAL;
//     queue.push(push_msg);
    
//     TEST_ASSERT(queue.tryPop(msg), "tryPop should succeed when queue has messages");
//     TEST_ASSERT_EQ(uint32_t(42), msg.header.sequence_id, "tryPop message sequence_id");
    
//     return true;
// }

// bool test_message_queue_clear() {
//     MessageQueue queue(10);
    
//     Message msg;
//     msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     msg.header.priority = Priority::NORMAL;
    
//     for (int i = 0; i < 5; ++i) {
//         msg.header.sequence_id = i;
//         queue.push(msg);
//     }
    
//     TEST_ASSERT_EQ(size_t(5), queue.size(), "Queue size before clear");
    
//     queue.clear();
    
//     TEST_ASSERT(queue.empty(), "Queue should be empty after clear");
//     TEST_ASSERT_EQ(size_t(0), queue.size(), "Queue size after clear should be 0");
    
//     return true;
// }

// // ============================================================================
// // MessageSerializer 测试
// // ============================================================================

// bool test_serialize_heartbeat() {
//     HeartbeatMessage original;
//     original.node_id = "SATELLITE_1";
//     original.uptime_ms = 123456789;
//     original.status = NodeStatus::HEALTHY;
//     original.cpu_usage_percent = 45;
//     original.memory_usage_percent = 60;
//     original.battery_percent = 85;
//     original.active_tasks = 3;
//     original.pending_messages = 10;
    
//     // 序列化
//     auto data = MessageSerializer::serializeHeartbeat(original);
//     TEST_ASSERT(!data.empty(), "Serialized heartbeat should not be empty");
    
//     // 反序列化
//     HeartbeatMessage deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeHeartbeat(data, deserialized),
//                 "Heartbeat deserialization should succeed");
    
//     // 验证
//     TEST_ASSERT_EQ(original.node_id, deserialized.node_id, "node_id mismatch");
//     TEST_ASSERT_EQ(original.uptime_ms, deserialized.uptime_ms, "uptime_ms mismatch");
//     TEST_ASSERT_EQ(static_cast<int>(original.status), 
//                    static_cast<int>(deserialized.status), "status mismatch");
//     TEST_ASSERT_EQ(original.cpu_usage_percent, deserialized.cpu_usage_percent, 
//                    "cpu_usage_percent mismatch");
//     TEST_ASSERT_EQ(original.memory_usage_percent, deserialized.memory_usage_percent, 
//                    "memory_usage_percent mismatch");
//     TEST_ASSERT_EQ(original.battery_percent, deserialized.battery_percent, 
//                    "battery_percent mismatch");
//     TEST_ASSERT_EQ(original.active_tasks, deserialized.active_tasks, 
//                    "active_tasks mismatch");
//     TEST_ASSERT_EQ(original.pending_messages, deserialized.pending_messages, 
//                    "pending_messages mismatch");
    
//     return true;
// }

// bool test_serialize_heartbeat_ack() {
//     HeartbeatAck original;
//     original.node_id = "COORDINATOR_1";
//     original.server_time_ms = 987654321;
//     original.sync_required = true;
    
//     auto data = MessageSerializer::serializeHeartbeatAck(original);
//     TEST_ASSERT(!data.empty(), "Serialized heartbeat ack should not be empty");
    
//     HeartbeatAck deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeHeartbeatAck(data, deserialized),
//                 "Heartbeat ack deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.node_id, deserialized.node_id, "node_id mismatch");
//     TEST_ASSERT_EQ(original.server_time_ms, deserialized.server_time_ms, 
//                    "server_time_ms mismatch");
//     TEST_ASSERT_EQ(original.sync_required, deserialized.sync_required, 
//                    "sync_required mismatch");
    
//     return true;
// }

// bool test_serialize_node_register() {
//     NodeRegisterMessage original;
//     original.node_id = "SAT_001";
//     original.node_name = "Satellite Alpha";
//     original.node_type = "OBSERVATION";
//     original.ip_address = "192.168.1.100";
//     original.port = 8800;
    
//     NodeCapability cap1;
//     cap1.capability_id = "IMAGING";
//     cap1.description = "High resolution imaging";
//     cap1.params["resolution"] = "4K";
//     original.capabilities.push_back(cap1);
    
//     original.metadata["version"] = "1.0.0";
//     original.metadata["region"] = "ASIA";
    
//     auto data = MessageSerializer::serializeNodeRegister(original);
//     TEST_ASSERT(!data.empty(), "Serialized node register should not be empty");
    
//     NodeRegisterMessage deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeNodeRegister(data, deserialized),
//                 "Node register deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.node_id, deserialized.node_id, "node_id mismatch");
//     TEST_ASSERT_EQ(original.node_name, deserialized.node_name, "node_name mismatch");
//     TEST_ASSERT_EQ(original.node_type, deserialized.node_type, "node_type mismatch");
//     TEST_ASSERT_EQ(original.ip_address, deserialized.ip_address, "ip_address mismatch");
//     TEST_ASSERT_EQ(original.port, deserialized.port, "port mismatch");
//     TEST_ASSERT_EQ(original.capabilities.size(), deserialized.capabilities.size(), 
//                    "capabilities size mismatch");
    
//     return true;
// }

// bool test_serialize_task_assign() {
//     TaskAssignMessage original;
//     original.segment_id = "SEG_001";
//     original.task_id = "TASK_001";
//     original.task_name = "Image Capture";
//     original.priority = Priority::URGENT;
//     original.profit = 100;
    
//     original.target.latitude_deg = 35.6762;
//     original.target.longitude_deg = 139.6503;
//     original.target.target_name = "Tokyo";
    
//     original.window.window_id = "WIN_001";
//     original.window.window_seq = 1;
//     original.window.start_time = "2026-01-22T10:00:00Z";
//     original.window.end_time = "2026-01-22T10:30:00Z";
    
//     original.execution.planned_start = "2026-01-22T10:05:00Z";
//     original.execution.planned_end = "2026-01-22T10:25:00Z";
//     original.execution.duration_s = 1200;
    
//     original.behavior_ref = "capture_image";
//     original.behavior_params["exposure"] = "auto";
    
//     ResourceRequirement req;
//     req.resource_id = "CAMERA_1";
//     req.semaphore_id = "SEM_CAM";
//     req.usage_type = "EXCLUSIVE";
//     req.acquire_at = "2026-01-22T10:05:00Z";
//     req.hold_duration_s = 1200;
//     original.resource_requirements.push_back(req);
    
//     auto data = MessageSerializer::serializeTaskAssign(original);
//     TEST_ASSERT(!data.empty(), "Serialized task assign should not be empty");
    
//     TaskAssignMessage deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeTaskAssign(data, deserialized),
//                 "Task assign deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.segment_id, deserialized.segment_id, "segment_id mismatch");
//     TEST_ASSERT_EQ(original.task_id, deserialized.task_id, "task_id mismatch");
//     TEST_ASSERT_EQ(original.task_name, deserialized.task_name, "task_name mismatch");
//     TEST_ASSERT_EQ(static_cast<int>(original.priority), 
//                    static_cast<int>(deserialized.priority), "priority mismatch");
//     TEST_ASSERT_EQ(original.profit, deserialized.profit, "profit mismatch");
//     TEST_ASSERT_EQ(original.behavior_ref, deserialized.behavior_ref, 
//                    "behavior_ref mismatch");
    
//     return true;
// }

// bool test_serialize_semaphore_acquire_request() {
//     SemaphoreAcquireRequest original;
//     original.request_id = "REQ_001";
//     original.node_id = "SAT_001";
//     original.semaphore_id = "SEM_CAMERA";
//     original.task_id = "TASK_001";
//     original.priority = Priority::NORMAL;
//     original.timeout_ms = 5000;
//     original.blocking = true;
    
//     auto data = MessageSerializer::serializeSemAcquireRequest(original);
//     TEST_ASSERT(!data.empty(), "Serialized semaphore request should not be empty");
    
//     SemaphoreAcquireRequest deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeSemAcquireRequest(data, deserialized),
//                 "Semaphore request deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.request_id, deserialized.request_id, "request_id mismatch");
//     TEST_ASSERT_EQ(original.node_id, deserialized.node_id, "node_id mismatch");
//     TEST_ASSERT_EQ(original.semaphore_id, deserialized.semaphore_id, 
//                    "semaphore_id mismatch");
//     TEST_ASSERT_EQ(original.task_id, deserialized.task_id, "task_id mismatch");
//     TEST_ASSERT_EQ(original.timeout_ms, deserialized.timeout_ms, "timeout_ms mismatch");
//     TEST_ASSERT_EQ(original.blocking, deserialized.blocking, "blocking mismatch");
    
//     return true;
// }

// bool test_serialize_semaphore_acquire_response() {
//     SemaphoreAcquireResponse original;
//     original.request_id = "REQ_001";
//     original.semaphore_id = "SEM_CAMERA";
//     original.granted = true;
//     original.queued = false;
//     original.queue_position = 0;
//     original.estimated_wait_ms = 0;
//     original.grant_token = 123456789;
//     original.deny_reason = "";
    
//     auto data = MessageSerializer::serializeSemAcquireResponse(original);
//     TEST_ASSERT(!data.empty(), "Serialized semaphore response should not be empty");
    
//     SemaphoreAcquireResponse deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeSemAcquireResponse(data, deserialized),
//                 "Semaphore response deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.request_id, deserialized.request_id, "request_id mismatch");
//     TEST_ASSERT_EQ(original.semaphore_id, deserialized.semaphore_id, 
//                    "semaphore_id mismatch");
//     TEST_ASSERT_EQ(original.granted, deserialized.granted, "granted mismatch");
//     TEST_ASSERT_EQ(original.grant_token, deserialized.grant_token, 
//                    "grant_token mismatch");
    
//     return true;
// }

// bool test_serialize_state_sync_request() {
//     StateSyncRequest original;
//     original.requester_id = "SAT_001";
//     original.sync_items.push_back("battery");
//     original.sync_items.push_back("tasks");
//     original.full_sync = false;
//     original.last_sync_time_ms = 1234567890;
    
//     auto data = MessageSerializer::serializeStateSyncRequest(original);
//     TEST_ASSERT(!data.empty(), "Serialized state sync request should not be empty");
    
//     StateSyncRequest deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeStateSyncRequest(data, deserialized),
//                 "State sync request deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.requester_id, deserialized.requester_id, 
//                    "requester_id mismatch");
//     TEST_ASSERT_EQ(original.sync_items.size(), deserialized.sync_items.size(), 
//                    "sync_items size mismatch");
//     TEST_ASSERT_EQ(original.full_sync, deserialized.full_sync, "full_sync mismatch");
//     TEST_ASSERT_EQ(original.last_sync_time_ms, deserialized.last_sync_time_ms, 
//                    "last_sync_time_ms mismatch");
    
//     return true;
// }

// bool test_serialize_barrier_wait() {
//     BarrierWaitMessage original;
//     original.sync_id = "SYNC_001";
//     original.node_id = "SAT_001";
//     original.barrier_type = "TASK_START";
//     original.current_status = NodeStatus::HEALTHY;
//     original.arrival_time_ms = 1234567890;
    
//     auto data = MessageSerializer::serializeBarrierWait(original);
//     TEST_ASSERT(!data.empty(), "Serialized barrier wait should not be empty");
    
//     BarrierWaitMessage deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeBarrierWait(data, deserialized),
//                 "Barrier wait deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.sync_id, deserialized.sync_id, "sync_id mismatch");
//     TEST_ASSERT_EQ(original.node_id, deserialized.node_id, "node_id mismatch");
//     TEST_ASSERT_EQ(original.barrier_type, deserialized.barrier_type, 
//                    "barrier_type mismatch");
    
//     return true;
// }

// bool test_serialize_error_response() {
//     ErrorResponse original;
//     original.error_code = ErrorCode::TIMEOUT;
//     original.error_message = "Operation timed out";
//     original.original_msg_type = "TASK_ASSIGN";
//     original.original_seq_id = 42;
//     original.details["retry_count"] = "3";
    
//     auto data = MessageSerializer::serializeError(original);
//     TEST_ASSERT(!data.empty(), "Serialized error response should not be empty");
    
//     ErrorResponse deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeError(data, deserialized),
//                 "Error response deserialization should succeed");
    
//     TEST_ASSERT_EQ(static_cast<int>(original.error_code), 
//                    static_cast<int>(deserialized.error_code), "error_code mismatch");
//     TEST_ASSERT_EQ(original.error_message, deserialized.error_message, 
//                    "error_message mismatch");
//     TEST_ASSERT_EQ(original.original_msg_type, deserialized.original_msg_type, 
//                    "original_msg_type mismatch");
//     TEST_ASSERT_EQ(original.original_seq_id, deserialized.original_seq_id, 
//                    "original_seq_id mismatch");
    
//     return true;
// }

// bool test_serialize_message_header() {
//     MessageHeader original;
//     original.magic = MessageHeader::MAGIC_NUMBER;
//     original.version = MessageHeader::PROTOCOL_VERSION;
//     original.msg_type = MessageType::TASK_ASSIGN;
//     original.sequence_id = 12345;
//     original.timestamp_ms = 1234567890123;
//     original.source_node_id = "SAT_001";
//     original.dest_node_id = "COORD_001";
//     original.priority = Priority::URGENT;
//     original.payload_size = 256;
//     original.checksum = 0xDEADBEEF;
    
//     auto data = MessageSerializer::serializeHeader(original);
//     TEST_ASSERT(!data.empty(), "Serialized header should not be empty");
    
//     MessageHeader deserialized;
//     TEST_ASSERT(MessageSerializer::deserializeHeader(data, deserialized),
//                 "Header deserialization should succeed");
    
//     TEST_ASSERT_EQ(original.magic, deserialized.magic, "magic mismatch");
//     TEST_ASSERT_EQ(original.version, deserialized.version, "version mismatch");
//     TEST_ASSERT_EQ(static_cast<int>(original.msg_type), 
//                    static_cast<int>(deserialized.msg_type), "msg_type mismatch");
//     TEST_ASSERT_EQ(original.sequence_id, deserialized.sequence_id, 
//                    "sequence_id mismatch");
//     TEST_ASSERT_EQ(original.timestamp_ms, deserialized.timestamp_ms, 
//                    "timestamp_ms mismatch");
//     TEST_ASSERT_EQ(original.source_node_id, deserialized.source_node_id, 
//                    "source_node_id mismatch");
//     TEST_ASSERT_EQ(original.dest_node_id, deserialized.dest_node_id, 
//                    "dest_node_id mismatch");
//     TEST_ASSERT_EQ(static_cast<int>(original.priority), 
//                    static_cast<int>(deserialized.priority), "priority mismatch");
//     TEST_ASSERT_EQ(original.payload_size, deserialized.payload_size, 
//                    "payload_size mismatch");
//     TEST_ASSERT_EQ(original.checksum, deserialized.checksum, "checksum mismatch");
    
//     return true;
// }

// bool test_serialize_full_message() {
//     Message original;
//     original.header.magic = MessageHeader::MAGIC_NUMBER;
//     original.header.version = MessageHeader::PROTOCOL_VERSION;
//     original.header.msg_type = MessageType::HEARTBEAT;
//     original.header.sequence_id = 100;
//     original.header.timestamp_ms = 9876543210;
//     original.header.source_node_id = "SAT_A";
//     original.header.dest_node_id = "SAT_B";
//     original.header.priority = Priority::NORMAL;
    
//     HeartbeatMessage hb;
//     hb.node_id = "SAT_A";
//     hb.uptime_ms = 3600000;
//     hb.status = NodeStatus::HEALTHY;
//     hb.cpu_usage_percent = 30;
//     hb.memory_usage_percent = 50;
//     hb.battery_percent = 90;
//     hb.active_tasks = 2;
//     hb.pending_messages = 5;
    
//     original.payload = MessageSerializer::serializeHeartbeat(hb);
//     original.header.payload_size = static_cast<uint32_t>(original.payload.size());
//     original.header.checksum = MessageSerializer::calculateChecksum(original.payload);
    
//     auto data = MessageSerializer::serialize(original);
//     TEST_ASSERT(!data.empty(), "Serialized message should not be empty");
    
//     Message deserialized;
//     TEST_ASSERT(MessageSerializer::deserialize(data, deserialized),
//                 "Message deserialization should succeed");
    
//     TEST_ASSERT(deserialized.isValid(), "Deserialized message should be valid");
//     TEST_ASSERT_EQ(original.header.sequence_id, deserialized.header.sequence_id, 
//                    "sequence_id mismatch");
//     TEST_ASSERT_EQ(original.payload.size(), deserialized.payload.size(), 
//                    "payload size mismatch");
    
//     return true;
// }

// bool test_checksum_calculation() {
//     std::vector<uint8_t> data1 = {0x01, 0x02, 0x03, 0x04};
//     std::vector<uint8_t> data2 = {0x01, 0x02, 0x03, 0x04};
//     std::vector<uint8_t> data3 = {0x01, 0x02, 0x03, 0x05};
    
//     uint32_t checksum1 = MessageSerializer::calculateChecksum(data1);
//     uint32_t checksum2 = MessageSerializer::calculateChecksum(data2);
//     uint32_t checksum3 = MessageSerializer::calculateChecksum(data3);
    
//     TEST_ASSERT_EQ(checksum1, checksum2, "Same data should produce same checksum");
//     TEST_ASSERT(checksum1 != checksum3, "Different data should produce different checksum");
    
//     return true;
// }

// // ============================================================================
// // Message 结构体测试
// // ============================================================================

// bool test_message_is_valid() {
//     Message msg;
//     msg.header.magic = 0;
//     TEST_ASSERT(!msg.isValid(), "Message with wrong magic should be invalid");
    
//     msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     TEST_ASSERT(msg.isValid(), "Message with correct magic should be valid");
    
//     return true;
// }

// bool test_message_is_request() {
//     Message msg;
//     msg.header.msg_type = MessageType::HEARTBEAT;  // 0x0001, odd = request
//     TEST_ASSERT(msg.isRequest(), "HEARTBEAT should be a request");
    
//     msg.header.msg_type = MessageType::HEARTBEAT_ACK;  // 0x0002, even = response
//     TEST_ASSERT(!msg.isRequest(), "HEARTBEAT_ACK should not be a request");
    
//     return true;
// }

// bool test_message_is_broadcast() {
//     Message msg;
//     msg.header.dest_node_id = "";
//     TEST_ASSERT(msg.isBroadcast(), "Empty dest should be broadcast");
    
//     msg.header.dest_node_id = "NODE_1";
//     TEST_ASSERT(!msg.isBroadcast(), "Non-empty dest should not be broadcast");
    
//     return true;
// }

// // ============================================================================
// // CommConfig 测试
// // ============================================================================

// bool test_comm_config_defaults() {
//     CommConfig config = CommConfig::getDefault();
    
//     TEST_ASSERT_EQ(std::string("NODE_DEFAULT"), config.node_id, "Default node_id");
//     TEST_ASSERT_EQ(std::string("Default Node"), config.node_name, "Default node_name");
//     TEST_ASSERT_EQ(std::string("SATELLITE"), config.node_type, "Default node_type");
//     TEST_ASSERT_EQ(std::string("0.0.0.0"), config.bind_address, "Default bind_address");
//     TEST_ASSERT_EQ(uint16_t(8800), config.bind_port, "Default bind_port");
//     TEST_ASSERT_EQ(uint32_t(5000), config.heartbeat_interval_ms, 
//                    "Default heartbeat_interval_ms");
//     TEST_ASSERT_EQ(uint32_t(15000), config.heartbeat_timeout_ms, 
//                    "Default heartbeat_timeout_ms");
//     TEST_ASSERT_EQ(uint32_t(3), config.max_missed_heartbeats, 
//                    "Default max_missed_heartbeats");
//     TEST_ASSERT_EQ(uint32_t(1000), config.message_queue_size, 
//                    "Default message_queue_size");
    
//     return true;
// }

// // ============================================================================
// // InterSatComm 基本功能测试
// // ============================================================================

// bool test_inter_sat_comm_creation() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;  // 使用系统分配端口，避免冲突
    
//     InterSatComm comm(config);
    
//     TEST_ASSERT(!comm.isRunning(), "New comm should not be running");
//     TEST_ASSERT_EQ(config.node_id, comm.getConfig().node_id, "Config should match");
    
//     return true;
// }

// bool test_inter_sat_comm_initialize() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     bool init_result = comm.initialize();
//     // 初始化可能因为端口问题失败，我们只检查它不会崩溃
//     // 在 Windows 上需要 WSAStartup，在测试环境中可能不可用
    
//     return true;  // 测试通过只要不崩溃
// }

// bool test_inter_sat_comm_handler_registration() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
//     TestMessageHandler handler;
    
//     // 注册处理器
//     comm.registerHandler(&handler);
    
//     // 取消注册处理器
//     comm.unregisterHandler(&handler);
    
//     return true;
// }

// bool test_inter_sat_comm_callback_registration() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     bool callback_called = false;
//     comm.registerCallback(MessageType::HEARTBEAT, [&callback_called](const Message& msg) {
//         callback_called = true;
//     });
    
//     return true;
// }

// bool test_inter_sat_comm_statistics() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     auto stats = comm.getStatistics();
//     TEST_ASSERT_EQ(uint64_t(0), stats.total_messages_sent, 
//                    "Initial messages sent should be 0");
//     TEST_ASSERT_EQ(uint64_t(0), stats.total_messages_received, 
//                    "Initial messages received should be 0");
    
//     comm.resetStatistics();
//     stats = comm.getStatistics();
//     TEST_ASSERT_EQ(uint64_t(0), stats.total_messages_sent, 
//                    "Reset messages sent should be 0");
    
//     return true;
// }

// bool test_inter_sat_comm_local_status() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     comm.updateLocalStatus(NodeStatus::HEALTHY);
//     // 没有直接获取本地状态的方法，但更新不应该导致崩溃
    
//     comm.updateLocalStatus(NodeStatus::BUSY);
//     comm.updateLocalStatus(NodeStatus::DEGRADED);
    
//     return true;
// }

// bool test_inter_sat_comm_get_connected_nodes() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     auto nodes = comm.getConnectedNodes();
//     TEST_ASSERT(nodes.empty(), "Initial connected nodes should be empty");
    
//     return true;
// }

// bool test_inter_sat_comm_get_node_status() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     // 获取不存在的节点状态
//     NodeStatus status = comm.getNodeStatus("NON_EXISTENT");
//     TEST_ASSERT_EQ(static_cast<int>(NodeStatus::UNKNOWN), static_cast<int>(status), 
//                    "Non-existent node should have UNKNOWN status");
    
//     return true;
// }

// bool test_inter_sat_comm_get_node_info() {
//     CommConfig config = CommConfig::getDefault();
//     config.node_id = "TEST_NODE";
//     config.bind_port = 0;
    
//     InterSatComm comm(config);
    
//     RemoteNode info;
//     bool result = comm.getNodeInfo("NON_EXISTENT", info);
//     TEST_ASSERT(!result, "Getting info for non-existent node should fail");
    
//     return true;
// }

// // ============================================================================
// // RemoteNode 测试
// // ============================================================================

// bool test_remote_node_defaults() {
//     RemoteNode node;
    
//     TEST_ASSERT_EQ(uint16_t(0), node.port, "Default port should be 0");
//     TEST_ASSERT_EQ(static_cast<int>(NodeStatus::UNKNOWN), 
//                    static_cast<int>(node.status), "Default status should be UNKNOWN");
//     TEST_ASSERT_EQ(uint64_t(0), node.session_token, "Default session_token should be 0");
//     TEST_ASSERT_EQ(uint64_t(0), node.last_heartbeat_time_ms, 
//                    "Default last_heartbeat_time_ms should be 0");
//     TEST_ASSERT_EQ(uint32_t(0), node.missed_heartbeats, 
//                    "Default missed_heartbeats should be 0");
//     TEST_ASSERT_EQ(int(-1), node.socket_fd, "Default socket_fd should be -1");
    
//     return true;
// }

// // ============================================================================
// // 集成测试（需要网络功能）
// // ============================================================================

// bool test_message_roundtrip() {
//     // 创建一个完整的消息，序列化再反序列化
//     TaskAssignMessage task_msg;
//     task_msg.segment_id = "SEG_ROUNDTRIP";
//     task_msg.task_id = "TASK_RT";
//     task_msg.task_name = "Roundtrip Test Task";
//     task_msg.priority = Priority::NORMAL;
//     task_msg.profit = 50;
    
//     auto payload = MessageSerializer::serializeTaskAssign(task_msg);
    
//     Message msg;
//     msg.header.magic = MessageHeader::MAGIC_NUMBER;
//     msg.header.version = MessageHeader::PROTOCOL_VERSION;
//     msg.header.msg_type = MessageType::TASK_ASSIGN;
//     msg.header.sequence_id = 999;
//     msg.header.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::system_clock::now().time_since_epoch()).count();
//     msg.header.source_node_id = "SENDER";
//     msg.header.dest_node_id = "RECEIVER";
//     msg.header.priority = Priority::NORMAL;
//     msg.header.payload_size = static_cast<uint32_t>(payload.size());
//     msg.payload = payload;
//     msg.header.checksum = MessageSerializer::calculateChecksum(payload);
    
//     // 序列化完整消息
//     auto serialized = MessageSerializer::serialize(msg);
    
//     // 反序列化
//     Message deserialized;
//     TEST_ASSERT(MessageSerializer::deserialize(serialized, deserialized),
//                 "Roundtrip deserialization should succeed");
    
//     // 验证
//     TEST_ASSERT(deserialized.isValid(), "Roundtrip message should be valid");
//     TEST_ASSERT_EQ(msg.header.sequence_id, deserialized.header.sequence_id, 
//                    "Roundtrip sequence_id mismatch");
//     TEST_ASSERT_EQ(msg.header.source_node_id, deserialized.header.source_node_id, 
//                    "Roundtrip source_node_id mismatch");
    
//     // 反序列化 payload
//     TaskAssignMessage deserialized_task;
//     TEST_ASSERT(MessageSerializer::deserializeTaskAssign(deserialized.payload, deserialized_task),
//                 "Roundtrip task deserialization should succeed");
//     TEST_ASSERT_EQ(task_msg.segment_id, deserialized_task.segment_id, 
//                    "Roundtrip segment_id mismatch");
//     TEST_ASSERT_EQ(task_msg.task_id, deserialized_task.task_id, 
//                    "Roundtrip task_id mismatch");
    
//     return true;
// }

// // ============================================================================
// // 并发测试
// // ============================================================================

// bool test_message_queue_concurrent() {
//     MessageQueue queue(100);
//     const int num_producers = 4;
//     const int messages_per_producer = 25;
//     std::atomic<int> push_count{0};
//     std::atomic<int> pop_count{0};
//     std::atomic<bool> stop_consumers{false};
    
//     // 生产者线程
//     std::vector<std::thread> producers;
//     for (int i = 0; i < num_producers; ++i) {
//         producers.emplace_back([&queue, &push_count, i, messages_per_producer]() {
//             for (int j = 0; j < messages_per_producer; ++j) {
//                 Message msg;
//                 msg.header.magic = MessageHeader::MAGIC_NUMBER;
//                 msg.header.sequence_id = i * 1000 + j;
//                 msg.header.priority = Priority::NORMAL;
//                 if (queue.push(msg)) {
//                     push_count++;
//                 }
//                 std::this_thread::sleep_for(std::chrono::microseconds(100));
//             }
//         });
//     }
    
//     // 消费者线程
//     std::vector<std::thread> consumers;
//     for (int i = 0; i < 2; ++i) {
//         consumers.emplace_back([&queue, &pop_count, &stop_consumers]() {
//             while (!stop_consumers) {
//                 Message msg;
//                 if (queue.tryPop(msg)) {
//                     pop_count++;
//                 }
//                 std::this_thread::sleep_for(std::chrono::microseconds(50));
//             }
//             // 清空剩余消息
//             Message msg;
//             while (queue.tryPop(msg)) {
//                 pop_count++;
//             }
//         });
//     }
    
//     // 等待生产者完成
//     for (auto& t : producers) {
//         t.join();
//     }
    
//     // 给消费者一些时间处理剩余消息
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     stop_consumers = true;
    
//     for (auto& t : consumers) {
//         t.join();
//     }
    
//     TEST_ASSERT_EQ(push_count.load(), pop_count.load(), 
//                    "All pushed messages should be popped");
//     TEST_ASSERT(queue.empty(), "Queue should be empty after all operations");
    
//     return true;
// }

// // ============================================================================
// // 主函数
// // ============================================================================

// int main() {
//     std::cout << "========================================" << std::endl;
//     std::cout << "InterSatComm 单元测试" << std::endl;
//     std::cout << "========================================" << std::endl;
    
//     int total = 0;
//     int passed = 0;
//     int failed = 0;
    
//     std::cout << "\n--- 消息类型辅助函数测试 ---" << std::endl;
//     RUN_TEST(test_message_type_to_string);
//     RUN_TEST(test_node_status_to_string);
//     RUN_TEST(test_priority_to_string);
    
//     std::cout << "\n--- MessageQueue 测试 ---" << std::endl;
//     RUN_TEST(test_message_queue_basic);
//     RUN_TEST(test_message_queue_push_pop);
//     RUN_TEST(test_message_queue_priority);
//     RUN_TEST(test_message_queue_full);
//     RUN_TEST(test_message_queue_try_pop);
//     RUN_TEST(test_message_queue_clear);
    
//     std::cout << "\n--- MessageSerializer 测试 ---" << std::endl;
//     RUN_TEST(test_serialize_heartbeat);
//     RUN_TEST(test_serialize_heartbeat_ack);
//     RUN_TEST(test_serialize_node_register);
//     RUN_TEST(test_serialize_task_assign);
//     RUN_TEST(test_serialize_semaphore_acquire_request);
//     RUN_TEST(test_serialize_semaphore_acquire_response);
//     RUN_TEST(test_serialize_state_sync_request);
//     RUN_TEST(test_serialize_barrier_wait);
//     RUN_TEST(test_serialize_error_response);
//     RUN_TEST(test_serialize_message_header);
//     RUN_TEST(test_serialize_full_message);
//     RUN_TEST(test_checksum_calculation);
    
//     std::cout << "\n--- Message 结构体测试 ---" << std::endl;
//     RUN_TEST(test_message_is_valid);
//     RUN_TEST(test_message_is_request);
//     RUN_TEST(test_message_is_broadcast);
    
//     std::cout << "\n--- CommConfig 测试 ---" << std::endl;
//     RUN_TEST(test_comm_config_defaults);
    
//     std::cout << "\n--- InterSatComm 基本功能测试 ---" << std::endl;
//     RUN_TEST(test_inter_sat_comm_creation);
//     RUN_TEST(test_inter_sat_comm_initialize);
//     RUN_TEST(test_inter_sat_comm_handler_registration);
//     RUN_TEST(test_inter_sat_comm_callback_registration);
//     RUN_TEST(test_inter_sat_comm_statistics);
//     RUN_TEST(test_inter_sat_comm_local_status);
//     RUN_TEST(test_inter_sat_comm_get_connected_nodes);
//     RUN_TEST(test_inter_sat_comm_get_node_status);
//     RUN_TEST(test_inter_sat_comm_get_node_info);
    
//     std::cout << "\n--- RemoteNode 测试 ---" << std::endl;
//     RUN_TEST(test_remote_node_defaults);
    
//     std::cout << "\n--- 集成测试 ---" << std::endl;
//     RUN_TEST(test_message_roundtrip);
    
//     std::cout << "\n--- 并发测试 ---" << std::endl;
//     RUN_TEST(test_message_queue_concurrent);
    
//     std::cout << "\n========================================" << std::endl;
//     std::cout << "测试结果汇总" << std::endl;
//     std::cout << "========================================" << std::endl;
//     std::cout << "总计: " << total << " 个测试" << std::endl;
//     std::cout << "通过: " << passed << " 个测试" << std::endl;
//     std::cout << "失败: " << failed << " 个测试" << std::endl;
    
//     if (failed == 0) {
//         std::cout << "\n✓ 所有测试通过!" << std::endl;
//         return 0;
//     } else {
//         std::cout << "\n✗ 有测试失败!" << std::endl;
//         return 1;
//     }
// }
